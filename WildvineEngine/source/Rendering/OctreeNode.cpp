#include "Rendering/OctreeNode.h"

#include <cmath>

namespace {
    float halfValue(float a, float b) {
        return a + (b - a) * 0.5f;
    }
}

bool OctreeBounds::isValid() const {
    if (!std::isfinite(minimum.x) ||
        !std::isfinite(minimum.y) ||
        !std::isfinite(minimum.z) ||
        !std::isfinite(maximum.x) ||
        !std::isfinite(maximum.y) ||
        !std::isfinite(maximum.z)) {
        return false;
    }

    return minimum.x <= maximum.x &&
        minimum.y <= maximum.y &&
        minimum.z <= maximum.z;
}

EU::Vector3 OctreeBounds::center() const {
    return EU::Vector3(
        halfValue(minimum.x, maximum.x),
        halfValue(minimum.y, maximum.y),
        halfValue(minimum.z, maximum.z));
}

bool OctreeBounds::contains(
    const OctreeBounds& other,
    float epsilon) const {

    if (!isValid() || !other.isValid()) {
        return false;
    }

    return other.minimum.x >= minimum.x - epsilon &&
        other.minimum.y >= minimum.y - epsilon &&
        other.minimum.z >= minimum.z - epsilon &&
        other.maximum.x <= maximum.x + epsilon &&
        other.maximum.y <= maximum.y + epsilon &&
        other.maximum.z <= maximum.z + epsilon;
}

OctreeNode::OctreeNode(
    const OctreeBounds& bounds,
    int depth)
    : m_bounds(bounds),
      m_depth(depth) {
}

bool OctreeNode::isLeaf() const {
    return m_children[0] == nullptr;
}

int OctreeNode::findContainingChild(
    const OctreeBounds& bounds) const {

    if (isLeaf() || !bounds.isValid()) {
        return -1;
    }

    for (int childIndex = 0; childIndex < 8; ++childIndex) {
        if (m_children[childIndex] &&
            m_children[childIndex]->m_bounds.contains(bounds)) {
            return childIndex;
        }
    }

    return -1;
}

void OctreeNode::subdivide(
    int maxDepth,
    unsigned int capacity) {

    if (!isLeaf() || m_depth >= maxDepth) {
        return;
    }

    const EU::Vector3 middle = m_bounds.center();

    for (int childIndex = 0; childIndex < 8; ++childIndex) {
        const bool highX = (childIndex & 1) != 0;
        const bool highY = (childIndex & 2) != 0;
        const bool highZ = (childIndex & 4) != 0;

        OctreeBounds childBounds{};
        childBounds.minimum = EU::Vector3(
            highX ? middle.x : m_bounds.minimum.x,
            highY ? middle.y : m_bounds.minimum.y,
            highZ ? middle.z : m_bounds.minimum.z);
        childBounds.maximum = EU::Vector3(
            highX ? m_bounds.maximum.x : middle.x,
            highY ? m_bounds.maximum.y : middle.y,
            highZ ? m_bounds.maximum.z : middle.z);

        m_children[childIndex] =
            std::make_unique<OctreeNode>(
                childBounds,
                m_depth + 1);
    }

    // Repartimos los objetos existentes. Los que cruzan varias celdas se
    // mantienen en este nodo para que nunca aparezcan duplicados.
    std::vector<OctreeEntry> remaining;
    remaining.reserve(m_entries.size());

    for (const OctreeEntry& entry : m_entries) {
        const int childIndex = findContainingChild(entry.bounds);
        if (childIndex >= 0) {
            m_children[childIndex]->insert(
                entry,
                maxDepth,
                capacity);
        }
        else {
            remaining.push_back(entry);
        }
    }

    m_entries.swap(remaining);
}

void OctreeNode::insert(
    const OctreeEntry& entry,
    int maxDepth,
    unsigned int capacity) {

    if (!entry.entity ||
        !entry.bounds.isValid() ||
        !m_bounds.contains(entry.bounds)) {
        return;
    }

    ++m_subtreeEntryCount;

    if (!isLeaf()) {
        const int childIndex = findContainingChild(entry.bounds);
        if (childIndex >= 0) {
            m_children[childIndex]->insert(
                entry,
                maxDepth,
                capacity);
            return;
        }
    }

    m_entries.push_back(entry);

    if (isLeaf() &&
        m_entries.size() > capacity &&
        m_depth < maxDepth) {
        subdivide(maxDepth, capacity);
    }
}

void OctreeNode::collectAll(
    std::vector<Entity*>& outVisible) const {

    for (const OctreeEntry& entry : m_entries) {
        if (entry.entity) {
            outVisible.push_back(entry.entity);
        }
    }

    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child) {
            child->collectAll(outVisible);
        }
    }
}

void OctreeNode::appendInsideDebug(
    std::vector<OctreeDebugBox>& debugBoxes,
    int debugDepth) const {

    if (debugDepth < 0 || m_depth <= debugDepth) {
        OctreeDebugBox box{};
        box.bounds = m_bounds;
        box.depth = m_depth;
        box.classification = FrustumBoxResult::Inside;
        box.leaf = isLeaf();
        debugBoxes.push_back(box);
    }

    if (debugDepth >= 0 && m_depth >= debugDepth) {
        return;
    }

    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child && child->m_subtreeEntryCount > 0) {
            child->appendInsideDebug(debugBoxes, debugDepth);
        }
    }
}

void OctreeNode::query(
    const Frustum& frustum,
    std::vector<Entity*>& outVisible,
    OctreeQueryStats& stats,
    std::vector<OctreeDebugBox>* debugBoxes,
    int debugDepth) const {

    ++stats.testedNodes;

    const FrustumBoxResult nodeResult =
        frustum.classifyBox(
            m_bounds.minimum,
            m_bounds.maximum,
            XMMatrixIdentity());

    if (debugBoxes &&
        (debugDepth < 0 || m_depth <= debugDepth)) {
        OctreeDebugBox box{};
        box.bounds = m_bounds;
        box.depth = m_depth;
        box.classification = nodeResult;
        box.leaf = isLeaf();
        debugBoxes->push_back(box);
    }

    // Frustum invalido: politica conservadora, nada desaparece.
    if (nodeResult == FrustumBoxResult::Invalid) {
        collectAll(outVisible);
        return;
    }

    // Todo el nodo esta fuera: descartamos el subarbol completo.
    if (nodeResult == FrustumBoxResult::Outside) {
        ++stats.culledNodes;
        return;
    }

    // Todo el nodo esta dentro: aceptamos su subarbol sin hacer pruebas AABB
    // individuales. Esta es una de las ganancias principales del Octree.
    if (nodeResult == FrustumBoxResult::Inside) {
        ++stats.acceptedNodes;
        collectAll(outVisible);

        if (debugBoxes &&
            (debugDepth < 0 || m_depth < debugDepth)) {
            for (const std::unique_ptr<OctreeNode>& child : m_children) {
                if (child && child->m_subtreeEntryCount > 0) {
                    child->appendInsideDebug(*debugBoxes, debugDepth);
                }
            }
        }
        return;
    }

    // Nodo intersectando: solo sus objetos directos requieren prueba AABB.
    for (const OctreeEntry& entry : m_entries) {
        if (!entry.entity) {
            continue;
        }

        ++stats.objectTests;
        const FrustumBoxResult objectResult =
            frustum.classifyBox(
                entry.bounds.minimum,
                entry.bounds.maximum,
                XMMatrixIdentity());

        if (objectResult != FrustumBoxResult::Outside) {
            outVisible.push_back(entry.entity);
        }
    }

    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child && child->m_subtreeEntryCount > 0) {
            child->query(
                frustum,
                outVisible,
                stats,
                debugBoxes,
                debugDepth);
        }
    }
}

unsigned int OctreeNode::countNodes() const {
    unsigned int result = 1;
    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child) {
            result += child->countNodes();
        }
    }
    return result;
}

unsigned int OctreeNode::countLeaves() const {
    if (isLeaf()) {
        return 1;
    }

    unsigned int result = 0;
    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child) {
            result += child->countLeaves();
        }
    }
    return result;
}
