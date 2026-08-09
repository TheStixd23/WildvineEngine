#pragma once

#include "Rendering/OctreeNode.h"

#include <memory>
#include <vector>

/**
 * @struct OctreeStatistics
 * @brief Informacion de construccion y consulta del Octree del ultimo frame.
 */
struct OctreeStatistics {
    unsigned int totalEntries = 0;
    unsigned int totalNodes = 0;
    unsigned int leafNodes = 0;

    unsigned int testedNodes = 0;
    unsigned int culledNodes = 0;
    unsigned int acceptedNodes = 0;
    unsigned int objectTests = 0;

    float buildTimeMs = 0.0f;
    float queryTimeMs = 0.0f;
    bool rebuiltThisFrame = false;
};

/**
 * @class Octree
 * @brief Indice espacial dinamico reconstruido para el culling del editor.
 *
 * En esta version se reconstruye a partir de las AABB mundiales antes de cada
 * consulta. Es una estrategia simple y segura para un editor donde los actores
 * pueden moverse libremente con el gizmo.
 */
class Octree {
public:
    Octree() = default;
    ~Octree();

    void clear();

    void setConfig(int maxDepth, unsigned int capacity);
    int getMaxDepth() const { return m_maxDepth; }
    unsigned int getCapacity() const { return m_capacity; }

    void setDebugCapture(bool enabled, int depth);

    void rebuild(const std::vector<OctreeEntry>& entries);

    void query(
        const Frustum& frustum,
        std::vector<Entity*>& outVisible);

    const OctreeStatistics& getStatistics() const {
        return m_statistics;
    }

    const std::vector<OctreeDebugBox>& getDebugBoxes() const {
        return m_debugBoxes;
    }

    bool hasRoot() const {
        return m_root != nullptr;
    }

private:
    OctreeBounds calculateRootBounds(
        const std::vector<OctreeEntry>& entries) const;
    unsigned long long calculateSignature(
        const std::vector<OctreeEntry>& entries) const;

private:
    std::unique_ptr<OctreeNode> m_root;
    OctreeStatistics m_statistics{};
    std::vector<OctreeDebugBox> m_debugBoxes;

    int m_maxDepth = 5;
    unsigned int m_capacity = 8;
    bool m_captureDebug = false;
    int m_debugDepth = 2;
    unsigned long long m_lastSignature = 0ull;
    bool m_hasSignature = false;
};
