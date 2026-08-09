#include "Rendering/OctreeNode.h"

#include <algorithm>
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
    int depth,
    float looseness,
    bool rootNode)
    : m_bounds(bounds),
      m_depth(depth),
      m_looseness((std::max)(1.0f, looseness)) {

    // La raiz ya contiene toda la escena y no necesita expandirse. Los hijos
    // usan bounds ligeramente mayores para admitir objetos que cruzan el
    // limite exacto de una celda sin duplicarlos.
    m_looseBounds = rootNode ? m_bounds : calculateLooseBounds(m_bounds);
}

bool OctreeNode::hasAnyChild() const {
    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child) {
            return true;
        }
    }
    return false;
}

int OctreeNode::findContainingChild(
    const OctreeBounds& bounds) const {

    if (!m_subdivided || !bounds.isValid()) {
        return -1;
    }

    // En un Loose Octree elegimos UN solo hijo por el centro del objeto.
    // Luego verificamos que la caja completa quepa dentro del loose bound de
    // ese hijo. Esto permite solapamiento espacial sin duplicar entidades.
    const EU::Vector3 middle = m_bounds.center();
    const EU::Vector3 objectCenter = bounds.center();

    int childIndex = 0;
    if (objectCenter.x >= middle.x) childIndex |= 1;
    if (objectCenter.y >= middle.y) childIndex |= 2;
    if (objectCenter.z >= middle.z) childIndex |= 4;

    const OctreeBounds childTight = calculateChildBounds(childIndex);
    const OctreeBounds childLoose = calculateLooseBounds(childTight);
    return childLoose.contains(bounds) ? childIndex : -1;
}

OctreeBounds OctreeNode::calculateChildBounds(
    int childIndex) const {

    const EU::Vector3 middle = m_bounds.center();
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
    return childBounds;
}

OctreeBounds OctreeNode::calculateLooseBounds(
    const OctreeBounds& tightBounds) const {

    if (!tightBounds.isValid()) {
        return OctreeBounds{};
    }

    const EU::Vector3 center = tightBounds.center();
    const float halfX = (tightBounds.maximum.x - tightBounds.minimum.x) * 0.5f * m_looseness;
    const float halfY = (tightBounds.maximum.y - tightBounds.minimum.y) * 0.5f * m_looseness;
    const float halfZ = (tightBounds.maximum.z - tightBounds.minimum.z) * 0.5f * m_looseness;

    OctreeBounds loose{};
    loose.minimum = EU::Vector3(
        center.x - halfX,
        center.y - halfY,
        center.z - halfZ);
    loose.maximum = EU::Vector3(
        center.x + halfX,
        center.y + halfY,
        center.z + halfZ);
    return loose;
}

OctreeNode* OctreeNode::ensureChild(int childIndex) {
    if (childIndex < 0 || childIndex >= 8) {
        return nullptr;
    }

    if (!m_children[childIndex]) {
        m_children[childIndex] = std::make_unique<OctreeNode>(
            calculateChildBounds(childIndex),
            m_depth + 1,
            m_looseness,
            false);
    }

    return m_children[childIndex].get();
}

void OctreeNode::subdivide(
    int maxDepth,
    unsigned int capacity) {

    if (m_subdivided || m_depth >= maxDepth) {
        return;
    }

    m_subdivided = true;

    // Repartimos solo las entradas que caben por completo en una region.
    // Los hijos se crean bajo demanda, evitando ocho nodos vacios por cada
    // subdivision del arbol.
    std::vector<OctreeEntry> remaining;
    remaining.reserve(m_entries.size());

    for (const OctreeEntry& entry : m_entries) {
        const int childIndex = findContainingChild(entry.bounds);
        if (childIndex >= 0) {
            OctreeNode* child = ensureChild(childIndex);
            if (child) {
                child->insert(entry, maxDepth, capacity);
                continue;
            }
        }
        remaining.push_back(entry);
    }

    m_entries.swap(remaining);
}

void OctreeNode::insert(
    const OctreeEntry& entry,
    int maxDepth,
    unsigned int capacity) {

    if (!entry.entity ||
        !entry.bounds.isValid() ||
        !m_looseBounds.contains(entry.bounds)) {
        return;
    }

    ++m_subtreeEntryCount;

    if (m_subdivided) {
        const int childIndex = findContainingChild(entry.bounds);
        if (childIndex >= 0) {
            OctreeNode* child = ensureChild(childIndex);
            if (child) {
                child->insert(entry, maxDepth, capacity);
                return;
            }
        }
    }

    m_entries.push_back(entry);

    if (!m_subdivided &&
        m_entries.size() > capacity &&
        m_depth < maxDepth) {
        subdivide(maxDepth, capacity);
    }
}

void OctreeNode::collectAll(
    std::vector<Entity*>& outEntities) const {

    for (const OctreeEntry& entry : m_entries) {
        if (entry.entity) {
            outEntities.push_back(entry.entity);
        }
    }

    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child && child->m_subtreeEntryCount > 0) {
            child->collectAll(outEntities);
        }
    }
}

void OctreeNode::appendInsideDebug(
    std::vector<OctreeDebugBox>& debugBoxes,
    int debugDepth) const {

    if (m_subtreeEntryCount == 0) {
        return;
    }

    if (debugDepth < 0 || m_depth <= debugDepth) {
        OctreeDebugBox box{};
        box.bounds = m_looseBounds;
        box.depth = m_depth;
        box.classification = FrustumBoxResult::Inside;
        box.leaf = !hasAnyChild();
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
    int debugDepth,
    std::vector<Entity*>* outNeedsRefinement) const {

    if (m_subtreeEntryCount == 0) {
        return;
    }

    ++stats.testedNodes;

    const FrustumBoxResult nodeResult = frustum.classifyBox(
        m_looseBounds.minimum,
        m_looseBounds.maximum,
        XMMatrixIdentity());

    if (debugBoxes &&
        (debugDepth < 0 || m_depth <= debugDepth)) {
        OctreeDebugBox box{};
        box.bounds = m_looseBounds;
        box.depth = m_depth;
        box.classification = nodeResult;
        box.leaf = !hasAnyChild();
        debugBoxes->push_back(box);
    }

    // Frustum invalido: politica conservadora. Se aceptan los candidatos y
    // se marcan para una comprobacion exacta posterior si el caller la usa.
    if (nodeResult == FrustumBoxResult::Invalid) {
        collectAll(outVisible);
        stats.intersectingEntries += m_subtreeEntryCount;
        if (outNeedsRefinement) {
            collectAll(*outNeedsRefinement);
        }
        return;
    }

    // Una region totalmente fuera elimina el subarbol completo.
    if (nodeResult == FrustumBoxResult::Outside) {
        ++stats.culledNodes;
        stats.culledEntries += m_subtreeEntryCount;
        return;
    }

    // Si el nodo completo esta dentro, sus objetos tambien lo estan. No hace
    // falta realizar ninguna prueba individual.
    if (nodeResult == FrustumBoxResult::Inside) {
        ++stats.acceptedNodes;
        stats.acceptedEntries += m_subtreeEntryCount;
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

    // Nodo intersectando: probamos solamente las entradas guardadas en este
    // nodo. Una AABB mundial que tambien intersecta se marca para una prueba
    // final mas precisa usando los bounds locales transformados (OBB corners).
    for (const OctreeEntry& entry : m_entries) {
        if (!entry.entity) {
            continue;
        }

        ++stats.objectTests;
        const FrustumBoxResult objectResult = frustum.classifyBox(
            entry.bounds.minimum,
            entry.bounds.maximum,
            XMMatrixIdentity());

        if (objectResult == FrustumBoxResult::Outside) {
            ++stats.culledEntries;
            continue;
        }

        outVisible.push_back(entry.entity);

        if (objectResult == FrustumBoxResult::Inside) {
            ++stats.acceptedEntries;
        }
        else {
            ++stats.intersectingEntries;
            if (outNeedsRefinement) {
                outNeedsRefinement->push_back(entry.entity);
            }
        }
    }

    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child && child->m_subtreeEntryCount > 0) {
            child->query(
                frustum,
                outVisible,
                stats,
                debugBoxes,
                debugDepth,
                outNeedsRefinement);
        }
    }
}

unsigned int OctreeNode::countNodes() const {
    if (m_subtreeEntryCount == 0) {
        return 0;
    }

    unsigned int result = 1;
    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child && child->m_subtreeEntryCount > 0) {
            result += child->countNodes();
        }
    }
    return result;
}

unsigned int OctreeNode::countLeaves() const {
    if (m_subtreeEntryCount == 0) {
        return 0;
    }

    bool anyOccupiedChild = false;
    unsigned int result = 0;
    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child && child->m_subtreeEntryCount > 0) {
            anyOccupiedChild = true;
            result += child->countLeaves();
        }
    }

    return anyOccupiedChild ? result : 1u;
}

unsigned int OctreeNode::getMaxOccupiedDepth() const {
    unsigned int result = static_cast<unsigned int>(m_depth);
    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child && child->m_subtreeEntryCount > 0) {
            result = (std::max)(result, child->getMaxOccupiedDepth());
        }
    }
    return result;
}

unsigned int OctreeNode::countInternalEntries() const {
    const bool internal = hasAnyChild();
    unsigned int result = internal
        ? static_cast<unsigned int>(m_entries.size())
        : 0u;

    for (const std::unique_ptr<OctreeNode>& child : m_children) {
        if (child && child->m_subtreeEntryCount > 0) {
            result += child->countInternalEntries();
        }
    }
    return result;
}
