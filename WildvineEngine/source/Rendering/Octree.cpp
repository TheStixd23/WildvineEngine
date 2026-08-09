#include "Rendering/Octree.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {
    using Clock = std::chrono::high_resolution_clock;

    float elapsedMilliseconds(
        const Clock::time_point& begin,
        const Clock::time_point& end) {
        return std::chrono::duration<float, std::milli>(
            end - begin).count();
    }
}

Octree::~Octree() = default;

void Octree::clear() {
    m_root.reset();
    m_debugBoxes.clear();
    m_statistics = OctreeStatistics{};
    m_lastSignature = 0ull;
    m_hasSignature = false;
}

void Octree::setConfig(
    int maxDepth,
    unsigned int capacity,
    float looseness) {

    maxDepth = (std::max)(1, (std::min)(8, maxDepth));
    capacity = (std::max)(1u, (std::min)(64u, capacity));
    looseness = (std::max)(1.0f, (std::min)(2.0f, looseness));

    if (m_maxDepth != maxDepth ||
        m_capacity != capacity ||
        fabsf(m_looseness - looseness) > 0.0001f) {
        m_maxDepth = maxDepth;
        m_capacity = capacity;
        m_looseness = looseness;
        m_hasSignature = false;
    }
}

void Octree::setDebugCapture(
    bool enabled,
    int depth) {

    m_captureDebug = enabled;
    m_debugDepth = (std::max)(0, (std::min)(m_maxDepth, depth));

    if (!enabled) {
        m_debugBoxes.clear();
    }
}

OctreeBounds Octree::calculateRootBounds(
    const std::vector<OctreeEntry>& entries) const {

    OctreeBounds result{};
    if (entries.empty()) {
        return result;
    }

    const float largest = (std::numeric_limits<float>::max)();
    EU::Vector3 minimum(largest, largest, largest);
    EU::Vector3 maximum(-largest, -largest, -largest);
    bool foundValid = false;

    for (const OctreeEntry& entry : entries) {
        if (!entry.entity || !entry.bounds.isValid()) {
            continue;
        }

        foundValid = true;
        minimum.x = (std::min)(minimum.x, entry.bounds.minimum.x);
        minimum.y = (std::min)(minimum.y, entry.bounds.minimum.y);
        minimum.z = (std::min)(minimum.z, entry.bounds.minimum.z);
        maximum.x = (std::max)(maximum.x, entry.bounds.maximum.x);
        maximum.y = (std::max)(maximum.y, entry.bounds.maximum.y);
        maximum.z = (std::max)(maximum.z, entry.bounds.maximum.z);
    }

    if (!foundValid) {
        return OctreeBounds{};
    }

    const EU::Vector3 center(
        minimum.x + (maximum.x - minimum.x) * 0.5f,
        minimum.y + (maximum.y - minimum.y) * 0.5f,
        minimum.z + (maximum.z - minimum.z) * 0.5f);

    const float extentX = maximum.x - minimum.x;
    const float extentY = maximum.y - minimum.y;
    const float extentZ = maximum.z - minimum.z;
    float halfSize = (std::max)(
        extentX,
        (std::max)(extentY, extentZ)) * 0.5f;

    if (!std::isfinite(halfSize) || halfSize < 0.05f) {
        halfSize = 0.05f;
    }

    // Margen relativo + minimo para que objetos pegados al borde no queden
    // fuera por redondeos al reconstruir el arbol.
    const float padding = (std::max)(0.02f, halfSize * 0.015f);
    halfSize += padding;

    result.minimum = EU::Vector3(
        center.x - halfSize,
        center.y - halfSize,
        center.z - halfSize);
    result.maximum = EU::Vector3(
        center.x + halfSize,
        center.y + halfSize,
        center.z + halfSize);
    return result;
}

unsigned long long Octree::calculateSignature(
    const std::vector<OctreeEntry>& entries) const {

    // FNV-1a de 64 bits. Incluye identidad, AABB y configuracion.
    unsigned long long hash = 1469598103934665603ull;
    const unsigned long long prime = 1099511628211ull;

    auto mix = [&](unsigned long long value) {
        for (int byteIndex = 0; byteIndex < 8; ++byteIndex) {
            hash ^= (value >> (byteIndex * 8)) & 0xffull;
            hash *= prime;
        }
    };

    auto mixFloat = [&](float value) {
        std::uint32_t bits = 0u;
        static_assert(sizeof(bits) == sizeof(value),
            "float inesperado para firma del Octree");
        std::memcpy(&bits, &value, sizeof(bits));
        mix(static_cast<unsigned long long>(bits));
    };

    mix(static_cast<unsigned long long>(m_maxDepth));
    mix(static_cast<unsigned long long>(m_capacity));
    mixFloat(m_looseness);
    mix(static_cast<unsigned long long>(entries.size()));

    for (const OctreeEntry& entry : entries) {
        mix(static_cast<unsigned long long>(
            reinterpret_cast<std::uintptr_t>(entry.entity)));
        mixFloat(entry.bounds.minimum.x);
        mixFloat(entry.bounds.minimum.y);
        mixFloat(entry.bounds.minimum.z);
        mixFloat(entry.bounds.maximum.x);
        mixFloat(entry.bounds.maximum.y);
        mixFloat(entry.bounds.maximum.z);
    }

    return hash;
}

void Octree::rebuild(
    const std::vector<OctreeEntry>& entries) {

    const Clock::time_point signatureBegin = Clock::now();

    std::vector<OctreeEntry> validEntries;
    validEntries.reserve(entries.size());

    for (const OctreeEntry& entry : entries) {
        if (entry.entity && entry.bounds.isValid()) {
            validEntries.push_back(entry);
        }
    }

    const unsigned long long signature = calculateSignature(validEntries);
    const Clock::time_point signatureEnd = Clock::now();
    const float signatureTime = elapsedMilliseconds(
        signatureBegin,
        signatureEnd);

    if (m_hasSignature && signature == m_lastSignature) {
        m_statistics.rebuiltThisFrame = false;
        m_statistics.signatureTimeMs = signatureTime;
        m_statistics.buildTimeMs = 0.0f;
        return;
    }

    const Clock::time_point buildBegin = Clock::now();

    m_root.reset();
    m_debugBoxes.clear();
    m_statistics = OctreeStatistics{};
    m_statistics.rebuiltThisFrame = true;
    m_statistics.signatureTimeMs = signatureTime;
    m_statistics.totalEntries =
        static_cast<unsigned int>(validEntries.size());

    m_lastSignature = signature;
    m_hasSignature = true;

    if (!validEntries.empty()) {
        const OctreeBounds rootBounds = calculateRootBounds(validEntries);

        if (rootBounds.isValid()) {
            m_root = std::make_unique<OctreeNode>(
                rootBounds,
                0,
                m_looseness,
                true);

            for (const OctreeEntry& entry : validEntries) {
                m_root->insert(entry, m_maxDepth, m_capacity);
            }

            m_statistics.totalNodes = m_root->countNodes();
            m_statistics.leafNodes = m_root->countLeaves();
            m_statistics.maxDepthUsed = m_root->getMaxOccupiedDepth();
            m_statistics.internalEntries = m_root->countInternalEntries();
        }
    }

    m_statistics.buildTimeMs = elapsedMilliseconds(
        buildBegin,
        Clock::now());
}

void Octree::query(
    const Frustum& frustum,
    std::vector<Entity*>& outVisible,
    std::vector<Entity*>* outNeedsRefinement) {

    const Clock::time_point begin = Clock::now();

    outVisible.clear();
    if (outNeedsRefinement) {
        outNeedsRefinement->clear();
    }
    m_debugBoxes.clear();

    m_statistics.testedNodes = 0;
    m_statistics.culledNodes = 0;
    m_statistics.acceptedNodes = 0;
    m_statistics.objectTests = 0;
    m_statistics.acceptedEntries = 0;
    m_statistics.culledEntries = 0;
    m_statistics.intersectingEntries = 0;

    if (m_root) {
        OctreeQueryStats queryStats{};
        m_root->query(
            frustum,
            outVisible,
            queryStats,
            m_captureDebug ? &m_debugBoxes : nullptr,
            m_debugDepth,
            outNeedsRefinement);

        m_statistics.testedNodes = queryStats.testedNodes;
        m_statistics.culledNodes = queryStats.culledNodes;
        m_statistics.acceptedNodes = queryStats.acceptedNodes;
        m_statistics.objectTests = queryStats.objectTests;
        m_statistics.acceptedEntries = queryStats.acceptedEntries;
        m_statistics.culledEntries = queryStats.culledEntries;
        m_statistics.intersectingEntries = queryStats.intersectingEntries;
    }

    m_statistics.queryTimeMs = elapsedMilliseconds(begin, Clock::now());
}

void Octree::refreshDebug(const Frustum& debugFrustum) {
    m_debugBoxes.clear();

    if (!m_captureDebug || !m_root) {
        return;
    }

    std::vector<Entity*> dummyVisible;
    OctreeQueryStats dummyStats{};
    m_root->query(
        debugFrustum,
        dummyVisible,
        dummyStats,
        &m_debugBoxes,
        m_debugDepth,
        nullptr);
}
