#pragma once

#include <chrono>

struct OctreeStatistics;

/**
 * @struct PerformanceStats
 * @brief Estadisticas de visibilidad generadas durante el ultimo frame.
 */
struct PerformanceStats {
    unsigned int totalRenderableObjects = 0;
    unsigned int visibleObjects = 0;
    unsigned int culledObjects = 0;
    unsigned int objectsWithoutBounds = 0;

    unsigned int totalSubmeshes = 0;
    unsigned int visibleSubmeshes = 0;
    unsigned int culledSubmeshes = 0;

    unsigned long long totalTriangles = 0;
    unsigned long long visibleTriangles = 0;
    unsigned long long culledTriangles = 0;

    float cullingTimeMs = 0.0f;
    bool frustumCullingEnabled = true;
    bool octreeEnabled = false;

    unsigned int octreeEntries = 0;
    unsigned int octreeTotalNodes = 0;
    unsigned int octreeLeafNodes = 0;
    unsigned int octreeTestedNodes = 0;
    unsigned int octreeCulledNodes = 0;
    unsigned int octreeAcceptedNodes = 0;
    unsigned int octreeObjectTests = 0;
    float octreeBuildTimeMs = 0.0f;
    float octreeQueryTimeMs = 0.0f;
    bool octreeRebuiltThisFrame = false;

    float getCullPercentage() const;
    float getTriangleCullPercentage() const;
};

/**
 * @class PerformanceProfiler
 * @brief Profiler ligero para medir el Frustum Culling.
 */
class PerformanceProfiler {
public:
    PerformanceProfiler() = default;

    /** Reinicia las estadisticas para un frame nuevo. */
    void beginFrame(bool frustumCullingEnabled, bool octreeEnabled = false);

    /** Inicia/finaliza la medicion del pase de culling. */
    void beginCulling();
    void endCulling();

    /** Registra el resultado de un objeto renderizable. */
    void recordRenderable(
        bool visible,
        bool hadBounds = true,
        unsigned int submeshCount = 0,
        unsigned long long triangleCount = 0);

    /** Copia al profiler las estadisticas espaciales del Octree. */
    void setOctreeStatistics(const OctreeStatistics& statistics);

    const PerformanceStats& getStats() const {
        return m_stats;
    }

private:
    using Clock = std::chrono::high_resolution_clock;

    PerformanceStats m_stats{};
    Clock::time_point m_cullingStart{};
    bool m_cullingActive = false;
};
