#include "Rendering/PerformanceProfiler.h"
#include "Rendering/Octree.h"

#include <algorithm>

float PerformanceStats::getCullPercentage() const {
    if (totalRenderableObjects == 0) {
        return 0.0f;
    }

    return (static_cast<float>(culledObjects) /
        static_cast<float>(totalRenderableObjects)) * 100.0f;
}

float PerformanceStats::getTriangleCullPercentage() const {
    if (totalTriangles == 0) {
        return 0.0f;
    }

    return (static_cast<float>(culledTriangles) /
        static_cast<float>(totalTriangles)) * 100.0f;
}

float PerformanceStats::getOctreeObjectTestAvoidancePercentage() const {
    if (octreeEntries == 0) {
        return 0.0f;
    }

    const unsigned int avoided = octreeEntries > octreeObjectTests
        ? octreeEntries - octreeObjectTests
        : 0u;

    return (static_cast<float>(avoided) /
        static_cast<float>(octreeEntries)) * 100.0f;
}

void PerformanceProfiler::beginFrame(
    bool frustumCullingEnabled,
    bool octreeEnabled) {

    m_stats = PerformanceStats{};
    m_stats.frustumCullingEnabled = frustumCullingEnabled;
    m_stats.octreeEnabled = frustumCullingEnabled && octreeEnabled;
    m_cullingActive = false;
}

void PerformanceProfiler::beginCulling() {
    m_cullingStart = Clock::now();
    m_cullingActive = true;
}

void PerformanceProfiler::endCulling() {
    if (!m_cullingActive) {
        return;
    }

    m_stats.cullingTimeMs =
        std::chrono::duration<float, std::milli>(
            Clock::now() - m_cullingStart).count();
    m_cullingActive = false;
}

void PerformanceProfiler::recordRenderable(
    bool visible,
    bool hadBounds,
    unsigned int submeshCount,
    unsigned long long triangleCount) {

    ++m_stats.totalRenderableObjects;
    m_stats.totalSubmeshes += submeshCount;
    m_stats.totalTriangles += triangleCount;

    if (!hadBounds) {
        ++m_stats.objectsWithoutBounds;
    }

    if (visible) {
        ++m_stats.visibleObjects;
        m_stats.visibleSubmeshes += submeshCount;
        m_stats.visibleTriangles += triangleCount;
    }
    else {
        ++m_stats.culledObjects;
        m_stats.culledSubmeshes += submeshCount;
        m_stats.culledTriangles += triangleCount;
    }
}

void PerformanceProfiler::setOctreeStatistics(
    const OctreeStatistics& statistics) {

    m_stats.octreeEntries = statistics.totalEntries;
    m_stats.octreeTotalNodes = statistics.totalNodes;
    m_stats.octreeLeafNodes = statistics.leafNodes;
    m_stats.octreeMaxDepthUsed = statistics.maxDepthUsed;
    m_stats.octreeInternalEntries = statistics.internalEntries;

    m_stats.octreeTestedNodes = statistics.testedNodes;
    m_stats.octreeCulledNodes = statistics.culledNodes;
    m_stats.octreeAcceptedNodes = statistics.acceptedNodes;
    m_stats.octreeObjectTests = statistics.objectTests;
    m_stats.octreeAcceptedEntries = statistics.acceptedEntries;
    m_stats.octreeCulledEntries = statistics.culledEntries;
    m_stats.octreeIntersectingEntries = statistics.intersectingEntries;

    m_stats.octreeSignatureTimeMs = statistics.signatureTimeMs;
    m_stats.octreeBuildTimeMs = statistics.buildTimeMs;
    m_stats.octreeQueryTimeMs = statistics.queryTimeMs;
    m_stats.octreeRebuiltThisFrame = statistics.rebuiltThisFrame;
}

void PerformanceProfiler::recordOctreeRefinement() {
    ++m_stats.octreeRefinementTests;
}

void PerformanceProfiler::recordOctreeValidation(bool matched) {
    ++m_stats.octreeValidationTests;
    if (!matched) {
        ++m_stats.octreeValidationMismatches;
    }
}

void PerformanceProfiler::recordParticleEmitter(
    bool visible,
    unsigned int activeParticles,
    unsigned int capacity,
    unsigned int spawnedThisFrame,
    float simulationTimeMs,
    float billboardTimeMs) {

    ++m_stats.particleEmitters;
    if (visible) {
        ++m_stats.visibleParticleEmitters;
    }
    m_stats.activeParticles += activeParticles;
    m_stats.particleCapacity += capacity;
    m_stats.particlesSpawnedThisFrame += spawnedThisFrame;
    m_stats.particleSimulationTimeMs += (std::max)(0.0f, simulationTimeMs);
    m_stats.particleBillboardTimeMs += (std::max)(0.0f, billboardTimeMs);
}
