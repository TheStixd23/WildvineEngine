#include "Rendering/RenderScene.h"

namespace {
    bool isFiniteValue(float value) {
        return std::isfinite(value) != 0;
    }

    float sanitizeNonNegative(float value, float fallback) {
        if (!isFiniteValue(value)) {
            return fallback;
        }
        return (std::max)(0.0f, value);
    }

    EU::Vector3 sanitizeColor(const EU::Vector3& color) {
        return EU::Vector3(
            sanitizeNonNegative(color.x, 1.0f),
            sanitizeNonNegative(color.y, 1.0f),
            sanitizeNonNegative(color.z, 1.0f));
    }

    EU::Vector3 sanitizePosition(const EU::Vector3& position) {
        return EU::Vector3(
            isFiniteValue(position.x) ? position.x : 0.0f,
            isFiniteValue(position.y) ? position.y : 0.0f,
            isFiniteValue(position.z) ? position.z : 0.0f);
    }

    EU::Vector3 sanitizeDirection(const EU::Vector3& direction) {
        EU::Vector3 result = direction;
        if (!isFiniteValue(result.x) ||
            !isFiniteValue(result.y) ||
            !isFiniteValue(result.z) ||
            result.isNearlyZero()) {
            return EU::Vector3(0.0f, -1.0f, 0.0f);
        }

        result = result.normalize();
        return result.isNearlyZero()
            ? EU::Vector3(0.0f, -1.0f, 0.0f)
            : result;
    }

    void sanitizeSpotAngles(LightData& light) {
        float innerAngle = isFiniteValue(light.innerSpotAngle)
            ? light.innerSpotAngle
            : 20.0f;
        float outerAngle = isFiniteValue(light.spotAngle)
            ? light.spotAngle
            : 35.0f;

        innerAngle = (std::max)(0.1f, (std::min)(innerAngle, 88.0f));
        outerAngle = (std::max)(innerAngle + 0.1f,
            (std::min)(outerAngle, 89.0f));

        light.innerSpotAngle = innerAngle;
        light.spotAngle = outerAngle;
    }
}

void RenderScene::clear() {
    opaqueObjects.clear();
    transparentObjects.clear();
    lights.clear();
    directionalLights.clear();
    skybox = nullptr;
}

void RenderScene::addLight(const LightData& light) {
    LightData sanitizedLight = light;

    sanitizedLight.color = sanitizeColor(light.color);
    sanitizedLight.position = sanitizePosition(light.position);
    sanitizedLight.direction = sanitizeDirection(light.direction);
    sanitizedLight.intensity = sanitizeNonNegative(light.intensity, 0.0f);

    const float sanitizedRange = isFiniteValue(light.range)
        ? light.range
        : 10.0f;
    sanitizedLight.range = (std::max)(kMinimumLightRange, sanitizedRange);

    sanitizeSpotAngles(sanitizedLight);

    if (!sanitizedLight.enabled ||
        sanitizedLight.intensity < kMinimumLightIntensity) {
        return;
    }

    lights.push_back(sanitizedLight);
    if (sanitizedLight.type == LightType::Directional) {
        directionalLights.push_back(sanitizedLight);
    }
}

const LightData* RenderScene::findPrimaryShadowLight() const {
    // Se prefiere una direccional porque el shader original fue construido
    // alrededor de este tipo de luz.
    for (const LightData& light : lights) {
        if (light.enabled && light.castShadow &&
            light.type == LightType::Directional) {
            return &light;
        }
    }

    // Un spotlight tambien puede usar un shadow map 2D mediante proyeccion.
    for (const LightData& light : lights) {
        if (light.enabled && light.castShadow &&
            light.type == LightType::Spot) {
            return &light;
        }
    }

    // Las point lights requieren cubemap de profundidad y no se fuerzan aqui.
    return nullptr;
}
