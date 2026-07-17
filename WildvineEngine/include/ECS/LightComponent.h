#pragma once
#include "Prerequisites.h"
#include "ECS/Component.h"
#include "ECS/Transform.h"
#include "Rendering/RenderTypes.h"

class DeviceContext;

class LightComponent : public Component {
public:
    LightComponent()
        : Component(ComponentType::LIGHT) {
        setType(LightType::Directional);
        setColor(EU::Vector3(1.0f, 1.0f, 1.0f));
        setIntensity(1.0f);
        setRange(10.0f);
        setSpotAngles(20.0f, 35.0f);
    }

    explicit LightComponent(LightType type)
        : LightComponent() {
        setType(type);
    }

    void init() override {}
    void update(float deltaTime) override { (void)deltaTime; }
    void render(DeviceContext& deviceContext) override { (void)deviceContext; }
    void destroy() override {}

    LightData& getLightData() { return m_light; }
    const LightData& getLightData() const { return m_light; }

    void setType(LightType type) {
        m_light.type = type;
        if (type == LightType::Directional && m_light.range <= 0.0f) {
            m_light.range = 10.0f;
        }
    }
    LightType getType() const { return m_light.type; }

    void setEnabled(bool enabled) { m_light.enabled = enabled; }
    bool isEnabled() const { return m_light.enabled; }

    void setColor(const EU::Vector3& color) {
        m_light.color.x = (std::max)(0.0f, color.x);
        m_light.color.y = (std::max)(0.0f, color.y);
        m_light.color.z = (std::max)(0.0f, color.z);
    }

    void setIntensity(float intensity) {
        m_light.intensity = (std::max)(0.0f, intensity);
    }

    void setRange(float range) {
        m_light.range = (std::max)(0.01f, range);
    }

    void setDirection(const EU::Vector3& direction) {
        const EU::Vector3 normalized = direction.normalize();
        if (!normalized.isNearlyZero()) {
            m_light.direction = normalized;
        }
    }

    void setPosition(const EU::Vector3& position) {
        m_light.position = position;
    }

    void setSpotAngles(float innerDegrees, float outerDegrees) {
        innerDegrees = (std::max)(0.1f, (std::min)(innerDegrees, 88.0f));
        outerDegrees = (std::max)(innerDegrees + 0.1f, (std::min)(outerDegrees, 89.0f));
        m_light.innerSpotAngle = innerDegrees;
        m_light.spotAngle = outerDegrees;
    }

    float getInnerSpotAngle() const { return m_light.innerSpotAngle; }
    float getOuterSpotAngle() const { return m_light.spotAngle; }

    void setCastShadow(bool value) {
        m_castShadow = value;
        m_light.castShadow = value;
    }
    bool canCastShadow() const { return m_castShadow; }

    void setFollowTransformPosition(bool value) { m_followTransformPosition = value; }
    void setFollowTransformDirection(bool value) { m_followTransformDirection = value; }
    bool followsTransformPosition() const { return m_followTransformPosition; }
    bool followsTransformDirection() const { return m_followTransformDirection; }

    void syncWithTransform(const Transform& transform) {
        if (m_followTransformPosition) {
            const XMMATRIX& world = transform.worldMatrix;
            XMFLOAT4X4 worldValues{};
            XMStoreFloat4x4(&worldValues, world);
            m_light.position = EU::Vector3(
                worldValues._41,
                worldValues._42,
                worldValues._43);
        }

        if (m_followTransformDirection) {
            setDirection(transform.getForwardVector());
        }
    }

private:
    LightData m_light;
    bool m_castShadow = false;
    bool m_followTransformPosition = true;
    bool m_followTransformDirection = false;
};
