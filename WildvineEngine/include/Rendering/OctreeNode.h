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

/**
 * @struct OctreeEntry
 * @brief Referencia no propietaria a una entidad y su AABB mundial.
 */
struct OctreeEntry {
    Entity* entity = nullptr;
    OctreeBounds bounds{};
};

/**
 * @struct OctreeQueryStats
 * @brief Estadisticas generadas durante una consulta del Octree.
 */
struct OctreeQueryStats {
    unsigned int testedNodes = 0;
    unsigned int culledNodes = 0;
    unsigned int acceptedNodes = 0;
    unsigned int objectTests = 0;
};

/**
 * @struct OctreeDebugBox
 * @brief Caja de un nodo almacenada para la visualizacion del editor.
 */
struct OctreeDebugBox {
    OctreeBounds bounds{};
    int depth = 0;
    FrustumBoxResult classification = FrustumBoxResult::Invalid;
    bool leaf = true;
};

/**
 * @class OctreeNode
 * @brief Nodo interno del Octree de WildvineEngine.
 *
 * Cada entidad se almacena exactamente una vez. Si su AABB cabe por completo
 * en un unico hijo, baja a ese hijo. Si cruza la frontera entre varios hijos,
 * permanece en el nodo actual. Esto evita duplicados durante el culling.
 */
class OctreeNode {
public:
    OctreeNode(const OctreeBounds& bounds, int depth);
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
        int debugDepth) const;

    unsigned int countNodes() const;
    unsigned int countLeaves() const;
    unsigned int getSubtreeEntryCount() const {
        return m_subtreeEntryCount;
    }

    const OctreeBounds& getBounds() const {
        return m_bounds;
    }

private:
    bool isLeaf() const;
    void subdivide(int maxDepth, unsigned int capacity);
    int findContainingChild(const OctreeBounds& bounds) const;
    void collectAll(std::vector<Entity*>& outVisible) const;
    void appendInsideDebug(
        std::vector<OctreeDebugBox>& debugBoxes,
        int debugDepth) const;

private:
    OctreeBounds m_bounds{};
    int m_depth = 0;
    unsigned int m_subtreeEntryCount = 0;
    std::vector<OctreeEntry> m_entries;
    std::array<std::unique_ptr<OctreeNode>, 8> m_children{};
};
