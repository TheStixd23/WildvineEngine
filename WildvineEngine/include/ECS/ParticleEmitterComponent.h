#pragma once

#include "Prerequisites.h"
#include "ECS/Component.h"
#include "Rendering/Particle.h"
#include "Rendering/Mesh.h"
#include "Rendering/MaterialInstance.h"
#include "Texture.h"

class Device;
class DeviceContext;
class Camera;
class Material;

enum class ParticleEmissionMode : int {
    Continuous = 0,
    Burst = 1
};

enum class ParticlePreset : int {
    Custom = 0,
    Smoke,
    Fire,
    Sparks,
    Dust
};

struct ParticleEmitterSettings {
    bool enabled = true;
    bool playing = true;
    bool loop = true;

    ParticleEmissionMode emissionMode = ParticleEmissionMode::Continuous;
    unsigned int maxParticles = 256;
    float spawnRate = 30.0f;
    unsigned int burstCount = 48;
    float duration = 5.0f;

    float lifetimeMin = 1.0f;
    float lifetimeMax = 2.0f;

    EU::Vector3 direction = EU::Vector3(0.0f, 1.0f, 0.0f);
    float speedMin = 0.5f;
    float speedMax = 1.5f;
    float spread = 0.25f;
    float spawnRadius = 0.05f;
    float gravityY = -0.25f;

    float startSize = 0.20f;
    float endSize = 0.35f;
    float angularVelocityMin = -0.5f;
    float angularVelocityMax = 0.5f;

    XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.85f);
    float emissiveStrength = 0.0f;
    // Recorta pixeles transparentes del PNG usando el AlphaCutoff que
    // ya entiende el shader PBR. Un valor bajo conserva bordes suaves.
    float alphaCutoff = 0.04f;
    bool additiveBlend = false;
};

/**
 * @class ParticleEmitterComponent
 * @brief Emisor CPU con pool fijo y render de billboards.
 *
 * La simulacion ocurre en CPU. Todas las particulas activas se compactan en
 * un solo buffer de vertices, por lo que cada emisor requiere un solo draw call.
 */
class ParticleEmitterComponent : public Component {
public:
    static constexpr unsigned int kGpuParticleCapacity = 2048;

    ParticleEmitterComponent();
    ~ParticleEmitterComponent() override = default;

    void init() override {}
    void update(float deltaTime) override;
    void render(DeviceContext& deviceContext) override { (void)deviceContext; }
    void destroy() override;

    HRESULT initialize(
        Device& device,
        Material* alphaMaterial,
        Material* additiveMaterial,
        Texture* fallbackAlbedo,
        Texture* fallbackNormal,
        Texture* fallbackMetallic,
        Texture* fallbackRoughness,
        Texture* fallbackAO,
        Texture* fallbackEmissive,
        ParticlePreset preset = ParticlePreset::Smoke);

    void buildBillboardMesh(
        DeviceContext& deviceContext,
        const Camera& camera,
        const XMMATRIX& emitterWorld);

    void play();
    void pause();
    void stop();
    void restart();
    void triggerBurst(unsigned int count = 0);

    void applyPreset(ParticlePreset preset);
    ParticlePreset getPreset() const { return m_preset; }

    bool setTexturePath(const std::string& path);
    const std::string& getTexturePath() const { return m_texturePath; }
    const std::string& getResolvedTexturePath() const { return m_resolvedTexturePath; }
    bool hasValidParticleTexture() const { return m_hasParticleTexture; }

    ParticleEmitterSettings& getSettings() { return m_settings; }
    const ParticleEmitterSettings& getSettings() const { return m_settings; }
    void sanitizeSettings();

    bool isEnabled() const { return m_settings.enabled; }
    void setEnabled(bool enabled) { m_settings.enabled = enabled; }
    bool isPlaying() const { return m_settings.playing; }

    Mesh* getMesh() { return &m_mesh; }
    MaterialInstance* getMaterialInstance() { return &m_materialInstance; }
    const MaterialInstance* getMaterialInstance() const { return &m_materialInstance; }

    unsigned int getActiveParticleCount() const { return m_activeParticleCount; }
    unsigned int getSpawnedThisFrame() const { return m_spawnedThisFrame; }
    unsigned int getCapacity() const {
        return (std::min)(m_settings.maxParticles, kGpuParticleCapacity);
    }

    float getLastSimulationTimeMs() const { return m_lastSimulationTimeMs; }
    float getLastBillboardTimeMs() const { return m_lastBillboardTimeMs; }

    bool isRenderReady() const {
        return m_initialized &&
            !m_mesh.getSubmeshes().empty() &&
            m_activeParticleCount > 0;
    }

    bool getLocalBounds(EU::Vector3& outMinimum, EU::Vector3& outMaximum) const;

    /** Copia parametros/preset, pero reinicia el pool de particulas. */
    void copyConfigurationFrom(const ParticleEmitterComponent& other);

private:
    bool createGpuMesh(Device& device);
    bool spawnOne();
    void deactivateAll();
    void updateLocalBounds();
    void refreshMaterialBindings();
    float randomRange(float minimum, float maximum);
    EU::Vector3 randomDirection();

private:
    Device* m_device = nullptr;
    Material* m_alphaMaterial = nullptr;
    Material* m_additiveMaterial = nullptr;

    Texture* m_fallbackAlbedo = nullptr;
    Texture* m_fallbackNormal = nullptr;
    Texture* m_fallbackMetallic = nullptr;
    Texture* m_fallbackRoughness = nullptr;
    Texture* m_fallbackAO = nullptr;
    Texture* m_fallbackEmissive = nullptr;

    Texture m_particleTexture;
    bool m_hasParticleTexture = false;
    std::string m_texturePath;
    std::string m_resolvedTexturePath;

    MaterialInstance m_materialInstance;
    Mesh m_mesh;

    std::vector<Particle> m_particles;
    std::vector<SimpleVertex> m_vertexScratch;

    ParticleEmitterSettings m_settings{};
    ParticlePreset m_preset = ParticlePreset::Custom;

    unsigned int m_activeParticleCount = 0;
    unsigned int m_spawnedThisFrame = 0;
    unsigned int m_pendingBurstCount = 0;
    unsigned int m_nextPoolSearch = 0;

    float m_spawnAccumulator = 0.0f;
    float m_playTime = 0.0f;

    EU::Vector3 m_localBoundsMin = EU::Vector3(-0.1f, -0.1f, -0.1f);
    EU::Vector3 m_localBoundsMax = EU::Vector3(0.1f, 0.1f, 0.1f);

    float m_lastSimulationTimeMs = 0.0f;
    float m_lastBillboardTimeMs = 0.0f;
    bool m_initialized = false;
};
