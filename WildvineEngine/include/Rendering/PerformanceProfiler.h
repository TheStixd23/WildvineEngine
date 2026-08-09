#pragma once

#include <chrono>

struct OctreeStatistics;

/** Estadisticas de visibilidad generadas durante el ultimo frame. */
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
    unsigned int octreeMaxDepthUsed = 0;
    unsigned int octreeInternalEntries = 0;

    unsigned int octreeTestedNodes = 0;
    unsigned int octreeCulledNodes = 0;
    unsigned int octreeAcceptedNodes = 0;
    unsigned int octreeObjectTests = 0;
    unsigned int octreeAcceptedEntries = 0;
    unsigned int octreeCulledEntries = 0;
    unsigned int octreeIntersectingEntries = 0;

    unsigned int octreeRefinementTests = 0;
    unsigned int octreeValidationTests = 0;
    unsigned int octreeValidationMismatches = 0;

    float octreeSignatureTimeMs = 0.0f;
    float octreeBuildTimeMs = 0.0f;
    float octreeQueryTimeMs = 0.0f;
    bool octreeRebuiltThisFrame = false;

    // Sistema de particulas
    unsigned int particleEmitters = 0;
    unsigned int visibleParticleEmitters = 0;
    unsigned int activeParticles = 0;
    unsigned int particleCapacity = 0;
    unsigned int particlesSpawnedThisFrame = 0;
    float particleSimulationTimeMs = 0.0f;
    float particleBillboardTimeMs = 0.0f;

    float getCullPercentage() const;
    float getTriangleCullPercentage() const;
    float getOctreeObjectTestAvoidancePercentage() const;
};

/** Profiler ligero para Frustum Culling, Octree y particulas. */
class PerformanceProfiler {
public:
    PerformanceProfiler() = default;

    void beginFrame(bool frustumCullingEnabled, bool octreeEnabled = false);

    void beginCulling();
    void endCulling();

    void recordRenderable(
        bool visible,
        bool hadBounds = true,
        unsigned int submeshCount = 0,
        unsigned long long triangleCount = 0);

    void setOctreeStatistics(const OctreeStatistics& statistics);
    void recordOctreeRefinement();
    void recordOctreeValidation(bool matched);

    void recordParticleEmitter(
        bool visible,
        unsigned int activeParticles,
        unsigned int capacity,
        unsigned int spawnedThisFrame,
        float simulationTimeMs,
        float billboardTimeMs);

    const PerformanceStats& getStats() const {
        return m_stats;
    }

private:
    using Clock = std::chrono::high_resolution_clock;

    PerformanceStats m_stats{};
    Clock::time_point m_cullingStart{};
    bool m_cullingActive = false;
};
