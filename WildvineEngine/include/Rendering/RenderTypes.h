/**
 * @file RenderTypes.h
 * @brief Tipos compartidos por el sistema de renderizado.
 */
#pragma once
#include "Prerequisites.h"

class Mesh;
class MaterialInstance;

enum class MaterialDomain {
    Opaque = 0,
    Masked,
    Transparent
};

enum class BlendMode {
    Opaque = 0,
    Alpha,
    Additive,
    PremultipliedAlpha
};

enum class RenderPassType {
    Shadow = 0,
    Opaque,
    Skybox,
    Transparent,
    Editor
};

enum class LightType {
    Directional = 0,
    Point,
    Spot
};

constexpr int kMaxSceneLights = 8;
constexpr float kMinimumLightRange = 0.01f;
constexpr float kMinimumLightIntensity = 0.0001f;

struct LightData {
    LightType type = LightType::Directional;
    EU::Vector3 color = EU::Vector3(1.0f, 1.0f, 1.0f);
    float intensity = 1.0f;

    EU::Vector3 direction = EU::Vector3(0.0f, -1.0f, 0.0f);
    float range = 10.0f;

    EU::Vector3 position = EU::Vector3(0.0f, 0.0f, 0.0f);
    // Los angulos se almacenan en grados y representan el semicono.
    float spotAngle = 35.0f;

    float innerSpotAngle = 20.0f;
    bool enabled = true;
    bool castShadow = false;
};

struct MaterialParams {
    XMFLOAT4 baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    float metallic = 0.0f;
    float roughness = 0.55f;
    float ao = 1.0f;
    float normalScale = 0.0f;
    float emissiveStrength = 0.0f;
    float alphaCutoff = 0.5f;
};

struct CBPerFrame {
    XMFLOAT4X4 View{};
    XMFLOAT4X4 Projection{};
    XMFLOAT4X4 LightViewProjection{};
    EU::Vector3 CameraPos{};
    float pad0 = 0.0f;
    EU::Vector3 LightDir = EU::Vector3(0.0f, -1.0f, 0.0f);
    float pad1 = 0.0f;
    EU::Vector3 LightColor = EU::Vector3(1.0f, 1.0f, 1.0f);
    float LightRange = 10.0f;
    EU::Vector3 LightPosition = EU::Vector3(0.0f, 3.0f, 0.0f);
    int LightType = 0;

    // Posicion.xyz + rango.
    XMFLOAT4 LightPositionsRanges[kMaxSceneLights]{};
    // Color.rgb con intensidad aplicada + tipo numerico.
    XMFLOAT4 LightColorsTypes[kMaxSceneLights]{};
    // Direccion normalizada.xyz + intensidad.
    XMFLOAT4 LightDirectionsIntensities[kMaxSceneLights]{};
    int LightCount = 0;
    XMFLOAT3 pad2 = XMFLOAT3(0.0f, 0.0f, 0.0f);

    // Se mantiene al final para conservar los offsets de los campos anteriores.
    // x = coseno del angulo interior, y = coseno del angulo exterior,
    // z = habilitada (1/0), w = proyecta sombra (1/0).
    XMFLOAT4 LightSpotAnglesEnabled[kMaxSceneLights]{};
};

static_assert((sizeof(CBPerFrame) % 16) == 0,
    "CBPerFrame debe tener un tamano multiplo de 16 bytes.");

struct CBPerObject {
    XMFLOAT4X4 World{};
};

struct CBPerMaterial {
    XMFLOAT4 BaseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    float Metallic = 0.0f;
    float Roughness = 0.55f;
    float AO = 1.0f;
    float NormalScale = 0.0f;
    float EmissiveStrength = 0.0f;
    float AlphaCutoff = 0.0f;
    float pad0 = 0.0f;
    float pad1 = 0.0f;
    float pad2 = 0.0f;
    float pad3 = 0.0f;
    float pad4 = 0.0f;
    float pad5 = 0.0f;
};

struct RenderObject {
    Mesh* mesh = nullptr;
    MaterialInstance* materialInstance = nullptr;
    std::vector<MaterialInstance*> materialInstances;
    XMMATRIX world = XMMatrixIdentity();
    bool castShadow = true;
    bool receiveShadow = true;
    bool transparent = false;
    float distanceToCamera = 0.0f;
};
