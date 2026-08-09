#pragma once

#include "Prerequisites.h"
#include "Rendering/Frustum.h"

#include <array>
#include <memory>
#include <vector>

class Entity;

/**
 * @struct OctreeBounds
 * @brief AABB en espacio mundo usada por el Octree.
 */
struct OctreeBounds {
    EU::Vector3 minimum{};
    EU::Vector3 maximum{};

    bool isValid() const;
    EU::Vector3 center() const;
    bool contains(const OctreeBounds& other, float epsilon = 0.0001f) const;
};

/** Referencia no propietaria a una entidad y su AABB mundial. */
struct OctreeEntry {
    Entity* entity = nullptr;
    OctreeBounds bounds{};
};

/** Estadisticas generadas durante una consulta del Octree. */
struct OctreeQueryStats {
    unsigned int testedNodes = 0;
    unsigned int culledNodes = 0;
    unsigned int acceptedNodes = 0;
    unsigned int objectTests = 0;

    // Cantidad de objetos resueltos por regiones o por la AABB mundial.
    unsigned int acceptedEntries = 0;
    unsigned int culledEntries = 0;
    unsigned int intersectingEntries = 0;
};

/** Caja de un nodo almacenada para la visualizacion del editor. */
struct OctreeDebugBox {
    OctreeBounds bounds{};
    int depth = 0;
    FrustumBoxResult classification = FrustumBoxResult::Invalid;
    bool leaf = true;
};

/**
 * @class OctreeNode
 * @brief Nodo espacial del Octree de WildvineEngine.
 *
 * Cada entidad se almacena exactamente una vez. Los hijos se crean de forma
 * perezosa: una region vacia no consume un nodo real. Si una AABB cruza el
 * centro de varias regiones se conserva en el padre para evitar duplicados.
 */
class OctreeNode {
public:
    OctreeNode(const OctreeBounds& bounds, int depth, float looseness = 1.35f, bool rootNode = false);
    ~OctreeNode() = default;

    void insert(
        const OctreeEntry& entry,
        int maxDepth,
        unsigned int capacity);

    void query(
        const Frustum& frustum,
        std::vector<Entity*>& outVisible,
        OctreeQueryStats& stats,
        std::vector<OctreeDebugBox>* debugBoxes,
        int debugDepth,
        std::vector<Entity*>* outNeedsRefinement = nullptr) const;

    unsigned int countNodes() const;
    unsigned int countLeaves() const;
    unsigned int getMaxOccupiedDepth() const;
    unsigned int countInternalEntries() const;

    unsigned int getSubtreeEntryCount() const {
        return m_subtreeEntryCount;
    }

    const OctreeBounds& getBounds() const {
        return m_bounds;
    }

private:
    bool hasAnyChild() const;
    int findContainingChild(const OctreeBounds& bounds) const;
    OctreeBounds calculateChildBounds(int childIndex) const;
    OctreeBounds calculateLooseBounds(const OctreeBounds& tightBounds) const;
    OctreeNode* ensureChild(int childIndex);
    void subdivide(int maxDepth, unsigned int capacity);

    void collectAll(std::vector<Entity*>& outEntities) const;
    void appendInsideDebug(
        std::vector<OctreeDebugBox>& debugBoxes,
        int debugDepth) const;

private:
    // m_bounds es la celda matematica; m_looseBounds es la caja realmente
    // usada para contener/cullar objetos del Loose Octree.
    OctreeBounds m_bounds{};
    OctreeBounds m_looseBounds{};
    int m_depth = 0;
    float m_looseness = 1.35f;
    unsigned int m_subtreeEntryCount = 0;
    bool m_subdivided = false;
    std::vector<OctreeEntry> m_entries;
    std::array<std::unique_ptr<OctreeNode>, 8> m_children{};
};
