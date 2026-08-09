#include "ECS/ParticleEmitterComponent.h"

#include "Device.h"
#include "DeviceContext.h"
#include "EngineUtilities/Utilities/Camera.h"
#include "MeshComponent.h"
#include "Rendering/Material.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <limits>
#include <random>

namespace {
    using ParticleClock = std::chrono::high_resolution_clock;

    std::mt19937& ParticleRandomEngine() {
        static std::mt19937 engine(0x57A11D1u);
        return engine;
    }

    const char* PresetTexturePath(ParticlePreset preset) {
        switch (preset) {
        case ParticlePreset::Smoke:
            return "Assets/Particles/particle_smoke.png";
        case ParticlePreset::Fire:
            return "Assets/Particles/particle_fire.png";
        case ParticlePreset::Sparks:
            return "Assets/Particles/particle_spark.png";
        case ParticlePreset::Dust:
            return "Assets/Particles/particle_dust.png";
        case ParticlePreset::Custom:
        default:
            return "Assets/Particles/particle_soft.png";
        }
    }

    std::filesystem::path EnsurePngExtension(const std::filesystem::path& value) {
        if (value.empty()) {
            return value;
        }

        std::filesystem::path result = value;
        std::string extension = result.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (extension.empty()) {
            result += ".png";
        }
        return result;
    }

    std::string ResolveParticleTexturePath(const std::string& requestedPath) {
        namespace fs = std::filesystem;

        if (requestedPath.empty()) {
            return {};
        }

        std::error_code error;
        const fs::path requestedRaw = fs::path(requestedPath);
        const fs::path requested = EnsurePngExtension(requestedRaw);
        const fs::path fileName = requested.filename();
        const fs::path current = fs::current_path(error);

        std::vector<fs::path> candidates;
        candidates.reserve(40);

        auto addParticleFolders = [&](const fs::path& base) {
            if (base.empty()) return;
            candidates.push_back(base / "Assets" / "Particles" / fileName);
            candidates.push_back(base / "Assets" / "Textures" / "Particles" / fileName);
            candidates.push_back(base / "bin" / "Assets" / "Particles" / fileName);
            candidates.push_back(base / "bin" / "Assets" / "Textures" / "Particles" / fileName);
        };

        candidates.push_back(requested);

        if (!error) {
            candidates.push_back(current / requested);
            candidates.push_back(current / "bin" / requested);
            addParticleFolders(current);

            fs::path parent = current.parent_path();
            if (!parent.empty()) {
                candidates.push_back(parent / requested);
                candidates.push_back(parent / "bin" / requested);
                addParticleFolders(parent);

                fs::path grandParent = parent.parent_path();
                if (!grandParent.empty()) {
                    candidates.push_back(grandParent / requested);
                    candidates.push_back(grandParent / "bin" / requested);
                    addParticleFolders(grandParent);
                }
            }
        }

        char executablePath[MAX_PATH] = {};
        const DWORD executableLength = GetModuleFileNameA(
            nullptr, executablePath, static_cast<DWORD>(MAX_PATH));
        if (executableLength > 0 && executableLength < MAX_PATH) {
            const fs::path executableDirectory = fs::path(executablePath).parent_path();
            candidates.push_back(executableDirectory / requested);
            addParticleFolders(executableDirectory);

            const fs::path executableParent = executableDirectory.parent_path();
            if (!executableParent.empty()) {
                candidates.push_back(executableParent / requested);
                addParticleFolders(executableParent);
            }
        }

        for (const fs::path& candidate : candidates) {
            if (candidate.empty()) {
                continue;
            }

            error.clear();
            if (fs::exists(candidate, error) && !error &&
                fs::is_regular_file(candidate, error) && !error) {
                error.clear();
                const fs::path normalized = fs::weakly_canonical(candidate, error);
                return error ? candidate.lexically_normal().string() : normalized.string();
            }
        }

        return requested.string();
    }

    EU::Vector3 transformPoint(
        const EU::Vector3& value,
        const XMMATRIX& matrix) {

        const XMVECTOR transformed = XMVector3TransformCoord(
            XMVectorSet(value.x, value.y, value.z, 1.0f),
            matrix);
        XMFLOAT3 output{};
        XMStoreFloat3(&output, transformed);
        return EU::Vector3(output.x, output.y, output.z);
    }

    float distanceSquared(
        const EU::Vector3& a,
        const EU::Vector3& b) {
        return (a - b).magnitudeSquared();
    }
}

ParticleEmitterComponent::ParticleEmitterComponent()
    : Component(ComponentType::PARTICLE_EMITTER) {
}

HRESULT ParticleEmitterComponent::initialize(
    Device& device,
    Material* alphaMaterial,
    Material* additiveMaterial,
    Texture* fallbackAlbedo,
    Texture* fallbackNormal,
    Texture* fallbackMetallic,
    Texture* fallbackRoughness,
    Texture* fallbackAO,
    Texture* fallbackEmissive,
    ParticlePreset preset) {

    destroy();

    if (!device.m_device || !alphaMaterial || !additiveMaterial) {
        ERROR("ParticleEmitterComponent", "initialize",
            "Device o material base invalido.");
        return E_INVALIDARG;
    }

    m_device = &device;
    m_alphaMaterial = alphaMaterial;
    m_additiveMaterial = additiveMaterial;
    m_fallbackAlbedo = fallbackAlbedo;
    m_fallbackNormal = fallbackNormal;
    m_fallbackMetallic = fallbackMetallic;
    m_fallbackRoughness = fallbackRoughness;
    m_fallbackAO = fallbackAO;
    m_fallbackEmissive = fallbackEmissive;

    m_particles.assign(kGpuParticleCapacity, Particle{});
    m_vertexScratch.assign(
        static_cast<size_t>(kGpuParticleCapacity) * 4u,
        SimpleVertex{});

    if (!createGpuMesh(device)) {
        destroy();
        return E_FAIL;
    }

    m_initialized = true;
    applyPreset(preset);
    sanitizeSettings();
    refreshMaterialBindings();

    MESSAGE("ParticleEmitterComponent", "initialize",
        "Emisor de particulas inicializado");
    return S_OK;
}

bool ParticleEmitterComponent::createGpuMesh(Device& device) {
    MeshComponent meshData;
    meshData.m_name = "ParticleBillboards";
    meshData.m_materialSlot = 0;

    meshData.m_vertex.assign(
        static_cast<size_t>(kGpuParticleCapacity) * 4u,
        SimpleVertex{});
    meshData.m_index.reserve(
        static_cast<size_t>(kGpuParticleCapacity) * 6u);

    for (unsigned int particleIndex = 0;
        particleIndex < kGpuParticleCapacity;
        ++particleIndex) {

        const unsigned int baseVertex = particleIndex * 4u;
        meshData.m_index.push_back(baseVertex + 0u);
        meshData.m_index.push_back(baseVertex + 1u);
        meshData.m_index.push_back(baseVertex + 2u);
        meshData.m_index.push_back(baseVertex + 0u);
        meshData.m_index.push_back(baseVertex + 2u);
        meshData.m_index.push_back(baseVertex + 3u);
    }

    meshData.m_numVertex = static_cast<int>(meshData.m_vertex.size());
    meshData.m_numIndex = static_cast<int>(meshData.m_index.size());

    Submesh submesh{};
    HRESULT hr = submesh.vertexBuffer.init(
        device,
        meshData,
        D3D11_BIND_VERTEX_BUFFER);
    if (FAILED(hr)) {
        ERROR("ParticleEmitterComponent", "createGpuMesh",
            "No se pudo crear el vertex buffer.");
        return false;
    }

    hr = submesh.indexBuffer.init(
        device,
        meshData,
        D3D11_BIND_INDEX_BUFFER);
    if (FAILED(hr)) {
        submesh.vertexBuffer.destroy();
        ERROR("ParticleEmitterComponent", "createGpuMesh",
            "No se pudo crear el index buffer.");
        return false;
    }

    submesh.indexCount = 0;
    submesh.startIndex = 0;
    submesh.materialSlot = 0;
    m_mesh.getSubmeshes().push_back(std::move(submesh));
    return true;
}

void ParticleEmitterComponent::destroy() {
    m_particleTexture.destroy();
    m_mesh.destroy();
    m_particles.clear();
    m_vertexScratch.clear();

    m_materialInstance = MaterialInstance{};
    m_device = nullptr;
    m_alphaMaterial = nullptr;
    m_additiveMaterial = nullptr;
    m_fallbackAlbedo = nullptr;
    m_fallbackNormal = nullptr;
    m_fallbackMetallic = nullptr;
    m_fallbackRoughness = nullptr;
    m_fallbackAO = nullptr;
    m_fallbackEmissive = nullptr;

    m_hasParticleTexture = false;
    m_texturePath.clear();
    m_resolvedTexturePath.clear();
    m_activeParticleCount = 0;
    m_spawnedThisFrame = 0;
    m_pendingBurstCount = 0;
    m_spawnAccumulator = 0.0f;
    m_playTime = 0.0f;
    m_nextPoolSearch = 0;
    m_initialized = false;
}

void ParticleEmitterComponent::sanitizeSettings() {
    m_settings.maxParticles = (std::max)(
        1u,
        (std::min)(m_settings.maxParticles, kGpuParticleCapacity));

    m_settings.spawnRate = (std::max)(0.0f, m_settings.spawnRate);
    m_settings.burstCount = (std::max)(1u, m_settings.burstCount);
    m_settings.duration = (std::max)(0.05f, m_settings.duration);

    m_settings.lifetimeMin = (std::max)(0.01f, m_settings.lifetimeMin);
    m_settings.lifetimeMax = (std::max)(
        m_settings.lifetimeMin,
        m_settings.lifetimeMax);

    m_settings.speedMin = (std::max)(0.0f, m_settings.speedMin);
    m_settings.speedMax = (std::max)(m_settings.speedMin, m_settings.speedMax);
    m_settings.spread = (std::max)(0.0f, (std::min)(1.0f, m_settings.spread));
    m_settings.spawnRadius = (std::max)(0.0f, m_settings.spawnRadius);
    m_settings.startSize = (std::max)(0.001f, m_settings.startSize);
    m_settings.endSize = (std::max)(0.001f, m_settings.endSize);

    if (m_settings.direction.isNearlyZero()) {
        m_settings.direction = EU::Vector3(0.0f, 1.0f, 0.0f);
    }
    else {
        m_settings.direction = m_settings.direction.normalize();
    }

    m_settings.color.x = (std::max)(0.0f, m_settings.color.x);
    m_settings.color.y = (std::max)(0.0f, m_settings.color.y);
    m_settings.color.z = (std::max)(0.0f, m_settings.color.z);
    m_settings.color.w = (std::max)(0.0f, (std::min)(1.0f, m_settings.color.w));
    m_settings.emissiveStrength = (std::max)(0.0f, m_settings.emissiveStrength);
    m_settings.alphaCutoff = (std::max)(
        0.0f, (std::min)(0.95f, m_settings.alphaCutoff));

    const unsigned int allowed = getCapacity();
    for (unsigned int index = allowed;
        index < static_cast<unsigned int>(m_particles.size());
        ++index) {
        m_particles[index].active = false;
    }
}

float ParticleEmitterComponent::randomRange(
    float minimum,
    float maximum) {

    if (maximum <= minimum) {
        return minimum;
    }
    std::uniform_real_distribution<float> distribution(minimum, maximum);
    return distribution(ParticleRandomEngine());
}

EU::Vector3 ParticleEmitterComponent::randomDirection() {
    const EU::Vector3 base = m_settings.direction.normalize();
    EU::Vector3 randomOffset(
        randomRange(-1.0f, 1.0f),
        randomRange(-1.0f, 1.0f),
        randomRange(-1.0f, 1.0f));

    if (!randomOffset.isNearlyZero()) {
        randomOffset = randomOffset.normalize();
    }

    EU::Vector3 result = base + randomOffset * m_settings.spread;
    if (result.isNearlyZero()) {
        result = base;
    }
    return result.normalize();
}

bool ParticleEmitterComponent::spawnOne() {
    const unsigned int capacity = getCapacity();
    if (capacity == 0 || m_particles.empty()) {
        return false;
    }

    for (unsigned int attempt = 0; attempt < capacity; ++attempt) {
        const unsigned int index =
            (m_nextPoolSearch + attempt) % capacity;
        Particle& particle = m_particles[index];
        if (particle.active) {
            continue;
        }

        particle = Particle{};
        particle.active = true;
        particle.age = 0.0f;
        particle.lifetime = randomRange(
            m_settings.lifetimeMin,
            m_settings.lifetimeMax);
        particle.startSize = m_settings.startSize;
        particle.endSize = m_settings.endSize;
        particle.rotation = randomRange(0.0f, XM_2PI);
        particle.angularVelocity = randomRange(
            m_settings.angularVelocityMin,
            m_settings.angularVelocityMax);

        EU::Vector3 spawnOffset(
            randomRange(-1.0f, 1.0f),
            randomRange(-1.0f, 1.0f),
            randomRange(-1.0f, 1.0f));
        if (!spawnOffset.isNearlyZero()) {
            spawnOffset = spawnOffset.normalize() *
                randomRange(0.0f, m_settings.spawnRadius);
        }
        particle.position = spawnOffset;

        const float speed = randomRange(
            m_settings.speedMin,
            m_settings.speedMax);
        particle.velocity = randomDirection() * speed;

        m_nextPoolSearch = (index + 1u) % capacity;
        ++m_spawnedThisFrame;
        return true;
    }

    return false;
}

void ParticleEmitterComponent::update(float deltaTime) {
    const ParticleClock::time_point start = ParticleClock::now();
    m_spawnedThisFrame = 0;

    if (!m_initialized) {
        m_lastSimulationTimeMs = 0.0f;
        return;
    }

    sanitizeSettings();

    const float safeDeltaTime = (std::max)(
        0.0f,
        (std::min)(deltaTime, 0.1f));

    // Un emisor desactivado o pausado conserva su estado sin consumir CPU
    // de simulacion. Stop, en cambio, vacia explicitamente el pool.
    if (!m_settings.enabled || !m_settings.playing) {
        updateLocalBounds();
        const ParticleClock::time_point end = ParticleClock::now();
        m_lastSimulationTimeMs =
            std::chrono::duration<float, std::milli>(end - start).count();
        return;
    }

    if (m_settings.enabled && m_settings.playing) {
        m_playTime += safeDeltaTime;

        bool canSpawnContinuous =
            m_settings.emissionMode == ParticleEmissionMode::Continuous;

        if (canSpawnContinuous && !m_settings.loop &&
            m_playTime > m_settings.duration) {
            canSpawnContinuous = false;
        }

        if (canSpawnContinuous && m_settings.spawnRate > 0.0f) {
            m_spawnAccumulator += m_settings.spawnRate * safeDeltaTime;
            const unsigned int desiredSpawnCount =
                static_cast<unsigned int>(m_spawnAccumulator);
            if (desiredSpawnCount > 0) {
                m_spawnAccumulator -= static_cast<float>(desiredSpawnCount);
                for (unsigned int spawnIndex = 0;
                    spawnIndex < desiredSpawnCount;
                    ++spawnIndex) {
                    if (!spawnOne()) {
                        break;
                    }
                }
            }
        }

        if (m_pendingBurstCount > 0) {
            const unsigned int pending = m_pendingBurstCount;
            m_pendingBurstCount = 0;
            for (unsigned int spawnIndex = 0;
                spawnIndex < pending;
                ++spawnIndex) {
                if (!spawnOne()) {
                    break;
                }
            }
        }
    }

    const unsigned int capacity = getCapacity();
    m_activeParticleCount = 0;
    for (unsigned int index = 0; index < capacity; ++index) {
        Particle& particle = m_particles[index];
        if (!particle.active) {
            continue;
        }

        particle.age += safeDeltaTime;
        if (particle.age >= particle.lifetime) {
            particle.active = false;
            continue;
        }

        particle.velocity.y += m_settings.gravityY * safeDeltaTime;
        particle.position += particle.velocity * safeDeltaTime;
        particle.rotation += particle.angularVelocity * safeDeltaTime;
        ++m_activeParticleCount;
    }

    updateLocalBounds();

    const ParticleClock::time_point end = ParticleClock::now();
    m_lastSimulationTimeMs =
        std::chrono::duration<float, std::milli>(end - start).count();
}

void ParticleEmitterComponent::updateLocalBounds() {
    const float largest = (std::numeric_limits<float>::max)();
    EU::Vector3 minimum(largest, largest, largest);
    EU::Vector3 maximum(-largest, -largest, -largest);
    bool hasParticle = false;

    const unsigned int capacity = getCapacity();
    for (unsigned int index = 0; index < capacity; ++index) {
        const Particle& particle = m_particles[index];
        if (!particle.active) {
            continue;
        }

        const float radius = (std::max)(0.01f, particle.currentSize()) * 0.75f;
        minimum.x = (std::min)(minimum.x, particle.position.x - radius);
        minimum.y = (std::min)(minimum.y, particle.position.y - radius);
        minimum.z = (std::min)(minimum.z, particle.position.z - radius);
        maximum.x = (std::max)(maximum.x, particle.position.x + radius);
        maximum.y = (std::max)(maximum.y, particle.position.y + radius);
        maximum.z = (std::max)(maximum.z, particle.position.z + radius);
        hasParticle = true;
    }

    if (!hasParticle) {
        const float radius = (std::max)(0.1f, m_settings.spawnRadius + m_settings.startSize);
        minimum = EU::Vector3(-radius, -radius, -radius);
        maximum = EU::Vector3(radius, radius, radius);
    }

    m_localBoundsMin = minimum;
    m_localBoundsMax = maximum;
}

bool ParticleEmitterComponent::getLocalBounds(
    EU::Vector3& outMinimum,
    EU::Vector3& outMaximum) const {

    outMinimum = m_localBoundsMin;
    outMaximum = m_localBoundsMax;
    return std::isfinite(outMinimum.x) &&
        std::isfinite(outMinimum.y) &&
        std::isfinite(outMinimum.z) &&
        std::isfinite(outMaximum.x) &&
        std::isfinite(outMaximum.y) &&
        std::isfinite(outMaximum.z) &&
        outMinimum.x <= outMaximum.x &&
        outMinimum.y <= outMaximum.y &&
        outMinimum.z <= outMaximum.z;
}

void ParticleEmitterComponent::buildBillboardMesh(
    DeviceContext& deviceContext,
    const Camera& camera,
    const XMMATRIX& emitterWorld) {

    const ParticleClock::time_point start = ParticleClock::now();

    if (!m_initialized || m_mesh.getSubmeshes().empty()) {
        m_lastBillboardTimeMs = 0.0f;
        return;
    }

    refreshMaterialBindings();

    struct SortedParticle {
        const Particle* particle = nullptr;
        EU::Vector3 worldPosition;
        float distanceSquaredToCamera = 0.0f;
    };

    std::vector<SortedParticle> activeParticles;
    activeParticles.reserve(m_activeParticleCount);

    const EU::Vector3 cameraPosition = camera.getPosition();
    const unsigned int capacity = getCapacity();
    for (unsigned int index = 0; index < capacity; ++index) {
        const Particle& particle = m_particles[index];
        if (!particle.active) {
            continue;
        }

        SortedParticle item{};
        item.particle = &particle;
        item.worldPosition = transformPoint(particle.position, emitterWorld);
        item.distanceSquaredToCamera = distanceSquared(
            item.worldPosition,
            cameraPosition);
        activeParticles.push_back(item);
    }

    // Alpha blending necesita back-to-front. Para additive el orden no importa.
    if (!m_settings.additiveBlend) {
        std::sort(
            activeParticles.begin(),
            activeParticles.end(),
            [](const SortedParticle& left, const SortedParticle& right) {
                return left.distanceSquaredToCamera >
                    right.distanceSquaredToCamera;
            });
    }

    const EU::Vector3 cameraRight = camera.GetRight().normalize();
    const EU::Vector3 cameraUp = camera.GetUp().normalize();
    EU::Vector3 cameraForward = camera.GetForward().normalize();
    if (cameraForward.isNearlyZero()) {
        cameraForward = EU::Vector3(0.0f, 0.0f, 1.0f);
    }
    const EU::Vector3 normal = -cameraForward;

    const size_t visibleCount = (std::min)(
        activeParticles.size(),
        static_cast<size_t>(kGpuParticleCapacity));

    for (size_t particleIndex = 0;
        particleIndex < visibleCount;
        ++particleIndex) {

        const Particle& particle = *activeParticles[particleIndex].particle;
        const EU::Vector3 center = activeParticles[particleIndex].worldPosition;
        const float halfSize = particle.currentSize() * 0.5f;

        const float cosine = std::cos(particle.rotation);
        const float sine = std::sin(particle.rotation);
        const EU::Vector3 rotatedRight =
            cameraRight * cosine + cameraUp * sine;
        const EU::Vector3 rotatedUp =
            cameraUp * cosine - cameraRight * sine;

        const EU::Vector3 horizontal = rotatedRight * halfSize;
        const EU::Vector3 vertical = rotatedUp * halfSize;

        const EU::Vector3 positions[4] = {
            center - horizontal + vertical,
            center + horizontal + vertical,
            center + horizontal - vertical,
            center - horizontal - vertical
        };
        const EU::Vector2 uvs[4] = {
            EU::Vector2(0.0f, 0.0f),
            EU::Vector2(1.0f, 0.0f),
            EU::Vector2(1.0f, 1.0f),
            EU::Vector2(0.0f, 1.0f)
        };

        for (unsigned int vertexIndex = 0; vertexIndex < 4u; ++vertexIndex) {
            SimpleVertex& vertex =
                m_vertexScratch[particleIndex * 4u + vertexIndex];
            vertex.Position = positions[vertexIndex];
            vertex.Normal = normal;
            vertex.Tangent = rotatedRight;
            vertex.Bitangent = rotatedUp;
            vertex.TextureCoordinate = uvs[vertexIndex];
        }
    }

    Submesh& submesh = m_mesh.getSubmeshes()[0];
    submesh.indexCount = static_cast<unsigned int>(visibleCount) * 6u;

    if (visibleCount > 0 && submesh.vertexBuffer.m_buffer) {
        submesh.vertexBuffer.update(
            deviceContext,
            nullptr,
            0,
            nullptr,
            m_vertexScratch.data(),
            0,
            0);
    }

    m_activeParticleCount = static_cast<unsigned int>(visibleCount);

    const ParticleClock::time_point end = ParticleClock::now();
    m_lastBillboardTimeMs =
        std::chrono::duration<float, std::milli>(end - start).count();
}

void ParticleEmitterComponent::play() {
    m_settings.playing = true;
}

void ParticleEmitterComponent::pause() {
    m_settings.playing = false;
}

void ParticleEmitterComponent::deactivateAll() {
    for (Particle& particle : m_particles) {
        particle.active = false;
    }
    m_activeParticleCount = 0;
    m_spawnedThisFrame = 0;
    m_pendingBurstCount = 0;
    m_spawnAccumulator = 0.0f;
    updateLocalBounds();
    if (!m_mesh.getSubmeshes().empty()) {
        m_mesh.getSubmeshes()[0].indexCount = 0;
    }
}

void ParticleEmitterComponent::stop() {
    m_settings.playing = false;
    m_playTime = 0.0f;
    deactivateAll();
}

void ParticleEmitterComponent::restart() {
    deactivateAll();
    m_playTime = 0.0f;
    m_settings.playing = true;
    if (m_settings.emissionMode == ParticleEmissionMode::Burst) {
        triggerBurst();
    }
}

void ParticleEmitterComponent::triggerBurst(unsigned int count) {
    const unsigned int requested = count > 0
        ? count
        : m_settings.burstCount;
    m_pendingBurstCount += (std::min)(
        requested,
        kGpuParticleCapacity);
    m_settings.playing = true;
}

bool ParticleEmitterComponent::setTexturePath(
    const std::string& path) {

    m_texturePath = path;
    m_resolvedTexturePath.clear();
    m_hasParticleTexture = false;
    m_particleTexture.destroy();

    if (!m_device || path.empty()) {
        refreshMaterialBindings();
        return false;
    }

    const std::string resolvedPath = ResolveParticleTexturePath(path);

    // Texture::init de algunas versiones de Wildvine agrega la extension
    // internamente. Para ser compatibles con ambas variantes, retiramos
    // .png antes de llamar al cargador. Asi nunca terminamos buscando
    // particle_fire.png.png.
    std::filesystem::path loadPath = std::filesystem::path(resolvedPath);
    std::string extension = loadPath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension == ".png") {
        loadPath.replace_extension();
    }

    const HRESULT hr = m_particleTexture.init(
        *m_device,
        loadPath.string(),
        PNG);

    m_hasParticleTexture = SUCCEEDED(hr) &&
        m_particleTexture.m_textureFromImg != nullptr;

    if (m_hasParticleTexture) {
        m_resolvedTexturePath = resolvedPath;
        MESSAGE("ParticleEmitterComponent", "setTexturePath",
            "Textura de particula OK: " << resolvedPath.c_str());
    }
    else {
        m_resolvedTexturePath.clear();
        ERROR("ParticleEmitterComponent", "setTexturePath",
            "No se pudo cargar: " << path.c_str() <<
            " | Se usa fallback");
    }

    refreshMaterialBindings();
    return m_hasParticleTexture;
}

void ParticleEmitterComponent::refreshMaterialBindings() {
    Material* selectedMaterial = m_settings.additiveBlend
        ? m_additiveMaterial
        : m_alphaMaterial;
    m_materialInstance.setMaterial(selectedMaterial);

    Texture* particleAlbedo = m_hasParticleTexture
        ? &m_particleTexture
        : m_fallbackAlbedo;

    m_materialInstance.setAlbedo(particleAlbedo);
    m_materialInstance.setNormal(m_fallbackNormal);
    m_materialInstance.setMetallic(m_fallbackMetallic);
    m_materialInstance.setRoughness(m_fallbackRoughness);
    m_materialInstance.setAO(m_fallbackAO);

    // Para presets emisivos usamos la misma mascara RGBA del sprite.
    m_materialInstance.setEmissive(
        m_hasParticleTexture
            ? &m_particleTexture
            : m_fallbackEmissive);

    MaterialParams& params = m_materialInstance.getParams();
    params.baseColor = m_settings.color;
    params.metallic = 0.0f;
    params.roughness = 0.9f;
    params.ao = 1.0f;
    params.normalScale = 0.0f;
    params.emissiveStrength = m_settings.emissiveStrength;
    params.alphaCutoff = m_settings.alphaCutoff;
}

void ParticleEmitterComponent::applyPreset(ParticlePreset preset) {
    m_preset = preset;

    switch (preset) {
    case ParticlePreset::Smoke:
        m_settings.enabled = true;
        m_settings.playing = true;
        m_settings.loop = true;
        m_settings.emissionMode = ParticleEmissionMode::Continuous;
        m_settings.maxParticles = 420;
        m_settings.spawnRate = 35.0f;
        m_settings.burstCount = 40;
        m_settings.duration = 5.0f;
        m_settings.lifetimeMin = 2.0f;
        m_settings.lifetimeMax = 4.0f;
        m_settings.direction = EU::Vector3(0.0f, 1.0f, 0.0f);
        m_settings.speedMin = 0.20f;
        m_settings.speedMax = 0.65f;
        m_settings.spread = 0.45f;
        m_settings.spawnRadius = 0.15f;
        m_settings.gravityY = 0.03f;
        m_settings.startSize = 0.25f;
        m_settings.endSize = 1.15f;
        m_settings.angularVelocityMin = -0.35f;
        m_settings.angularVelocityMax = 0.35f;
        m_settings.color = XMFLOAT4(0.72f, 0.75f, 0.78f, 0.55f);
        m_settings.emissiveStrength = 0.0f;
        m_settings.alphaCutoff = 0.035f;
        m_settings.additiveBlend = false;
        break;

    case ParticlePreset::Fire:
        m_settings.enabled = true;
        m_settings.playing = true;
        m_settings.loop = true;
        m_settings.emissionMode = ParticleEmissionMode::Continuous;
        m_settings.maxParticles = 360;
        m_settings.spawnRate = 75.0f;
        m_settings.burstCount = 50;
        m_settings.duration = 5.0f;
        m_settings.lifetimeMin = 0.45f;
        m_settings.lifetimeMax = 1.10f;
        m_settings.direction = EU::Vector3(0.0f, 1.0f, 0.0f);
        m_settings.speedMin = 0.65f;
        m_settings.speedMax = 1.55f;
        m_settings.spread = 0.32f;
        m_settings.spawnRadius = 0.12f;
        m_settings.gravityY = 0.30f;
        m_settings.startSize = 0.32f;
        m_settings.endSize = 0.08f;
        m_settings.angularVelocityMin = -0.8f;
        m_settings.angularVelocityMax = 0.8f;
        m_settings.color = XMFLOAT4(1.0f, 0.72f, 0.30f, 0.88f);
        m_settings.emissiveStrength = 2.2f;
        m_settings.alphaCutoff = 0.025f;
        m_settings.additiveBlend = true;
        break;

    case ParticlePreset::Sparks:
        m_settings.enabled = true;
        m_settings.playing = true;
        m_settings.loop = false;
        m_settings.emissionMode = ParticleEmissionMode::Burst;
        m_settings.maxParticles = 300;
        m_settings.spawnRate = 0.0f;
        m_settings.burstCount = 80;
        m_settings.duration = 1.0f;
        m_settings.lifetimeMin = 0.35f;
        m_settings.lifetimeMax = 1.15f;
        m_settings.direction = EU::Vector3(0.0f, 1.0f, 0.0f);
        m_settings.speedMin = 2.0f;
        m_settings.speedMax = 5.5f;
        m_settings.spread = 0.95f;
        m_settings.spawnRadius = 0.04f;
        m_settings.gravityY = -5.5f;
        m_settings.startSize = 0.10f;
        m_settings.endSize = 0.015f;
        m_settings.angularVelocityMin = -2.0f;
        m_settings.angularVelocityMax = 2.0f;
        m_settings.color = XMFLOAT4(1.0f, 0.72f, 0.18f, 1.0f);
        m_settings.emissiveStrength = 4.0f;
        m_settings.alphaCutoff = 0.020f;
        m_settings.additiveBlend = true;
        break;

    case ParticlePreset::Dust:
        m_settings.enabled = true;
        m_settings.playing = true;
        m_settings.loop = true;
        m_settings.emissionMode = ParticleEmissionMode::Continuous;
        m_settings.maxParticles = 300;
        m_settings.spawnRate = 20.0f;
        m_settings.burstCount = 45;
        m_settings.duration = 5.0f;
        m_settings.lifetimeMin = 1.1f;
        m_settings.lifetimeMax = 2.4f;
        m_settings.direction = EU::Vector3(0.0f, 0.65f, 0.25f);
        m_settings.speedMin = 0.15f;
        m_settings.speedMax = 0.65f;
        m_settings.spread = 0.80f;
        m_settings.spawnRadius = 0.28f;
        m_settings.gravityY = -0.08f;
        m_settings.startSize = 0.18f;
        m_settings.endSize = 0.55f;
        m_settings.angularVelocityMin = -0.55f;
        m_settings.angularVelocityMax = 0.55f;
        m_settings.color = XMFLOAT4(0.72f, 0.58f, 0.42f, 0.52f);
        m_settings.emissiveStrength = 0.0f;
        m_settings.alphaCutoff = 0.035f;
        m_settings.additiveBlend = false;
        break;

    case ParticlePreset::Custom:
    default:
        break;
    }

    sanitizeSettings();
    setTexturePath(PresetTexturePath(preset));
    restart();
}

void ParticleEmitterComponent::copyConfigurationFrom(
    const ParticleEmitterComponent& other) {

    m_settings = other.m_settings;
    m_preset = other.m_preset;
    sanitizeSettings();
    setTexturePath(other.m_texturePath);
    restart();
}
