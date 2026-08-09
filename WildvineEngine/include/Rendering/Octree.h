#pragma once

#include "Rendering/OctreeNode.h"

#include <memory>
#include <vector>

/** Informacion de construccion y consulta del Octree del ultimo frame. */
struct OctreeStatistics {
    unsigned int totalEntries = 0;
    unsigned int totalNodes = 0;
    unsigned int leafNodes = 0;
    unsigned int maxDepthUsed = 0;
    unsigned int internalEntries = 0;

    unsigned int testedNodes = 0;
    unsigned int culledNodes = 0;
    unsigned int acceptedNodes = 0;
    unsigned int objectTests = 0;

    unsigned int acceptedEntries = 0;
    unsigned int culledEntries = 0;
    unsigned int intersectingEntries = 0;

    // signatureTimeMs mide solo la deteccion de cambios. buildTimeMs es cero
    // si el arbol se reutilizo sin reconstruirse.
    float signatureTimeMs = 0.0f;
    float buildTimeMs = 0.0f;
    float queryTimeMs = 0.0f;
    bool rebuiltThisFrame = false;
};

/**
 * @class Octree
 * @brief Indice espacial dinamico para culling de escenas 3D.
 *
 * El arbol se reconstruye solo cuando cambia la firma espacial (entidades,
 * AABB o configuracion). Mover exclusivamente la camara reutiliza el arbol.
 */
class Octree {
public:
    Octree() = default;
    ~Octree();

    void clear();

    void setConfig(int maxDepth, unsigned int capacity, float looseness = 1.35f);
    int getMaxDepth() const { return m_maxDepth; }
    unsigned int getCapacity() const { return m_capacity; }
    float getLooseness() const { return m_looseness; }

    void setDebugCapture(bool enabled, int depth);

    void rebuild(const std::vector<OctreeEntry>& entries);

    void query(
        const Frustum& frustum,
        std::vector<Entity*>& outVisible,
        std::vector<Entity*>* outNeedsRefinement = nullptr);

    /**
     * Recalcula solo las cajas de debug contra otro frustum (por ejemplo el
     * frustum congelado del editor) sin alterar las estadisticas reales.
     */
    void refreshDebug(const Frustum& debugFrustum);

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
    float m_looseness = 1.35f;
    bool m_captureDebug = false;
    int m_debugDepth = 2;
    unsigned long long m_lastSignature = 0ull;
    bool m_hasSignature = false;
};
