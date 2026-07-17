#pragma once
#include "Prerequisites.h"
#include "EngineUtilities/Vectors/Vector3.h"
#include "Component.h"

class Transform : public Component {
public:
    Transform()
        : Component(ComponentType::TRANSFORM),
          position(0.0f, 0.0f, 0.0f),
          rotation(0.0f, 0.0f, 0.0f),
          scale(1.0f, 1.0f, 1.0f),
          matrix(XMMatrixIdentity()),
          worldMatrix(XMMatrixIdentity()),
          m_dirty(true) {
    }

    void init() override {
        position.zero();
        rotation.zero();
        scale.one();
        matrix = XMMatrixIdentity();
        worldMatrix = XMMatrixIdentity();
        m_dirty = true;
    }

    void update(float deltaTime) override {
        (void)deltaTime;
        rebuildLocalMatrix();
    }

    void render(DeviceContext& deviceContext) override {
        (void)deviceContext;
    }

    void destroy() override {}

    const EU::Vector3& getPosition() const { return position; }
    const EU::Vector3& getRotation() const { return rotation; }
    const EU::Vector3& getScale() const { return scale; }

    EU::Vector3 getWorldPosition() const {
        XMFLOAT4X4 values{};
        XMStoreFloat4x4(&values, worldMatrix);
        return EU::Vector3(values._41, values._42, values._43);
    }

    void setPosition(const EU::Vector3& newPosition) {
        position = newPosition;
        markDirty();
    }

    void setRotation(const EU::Vector3& newRotation) {
        rotation = newRotation;
        markDirty();
    }

    void setScale(const EU::Vector3& newScale) {
        scale = sanitizeScale(newScale);
        markDirty();
    }

    void setTransform(const EU::Vector3& newPosition,
        const EU::Vector3& newRotation,
        const EU::Vector3& newScale) {
        position = newPosition;
        rotation = newRotation;
        scale = sanitizeScale(newScale);
        markDirty();
    }

    /**
     * @brief Reemplaza la transformacion local usando una matriz ya calculada.
     *
     * Se utiliza al reparentar entidades para conservar su posicion visual.
     * La matriz se descompone en escala, rotacion y traslacion para que el
     * inspector y el gizmo sigan mostrando valores editables.
     */
    bool setFromLocalMatrix(const XMMATRIX& localMatrix) {
        XMVECTOR scaleVector = XMVectorZero();
        XMVECTOR rotationQuaternion = XMQuaternionIdentity();
        XMVECTOR translationVector = XMVectorZero();

        if (!XMMatrixDecompose(
            &scaleVector,
            &rotationQuaternion,
            &translationVector,
            localMatrix)) {
            return false;
        }

        XMFLOAT3 scaleValues{};
        XMFLOAT3 translationValues{};
        XMStoreFloat3(&scaleValues, scaleVector);
        XMStoreFloat3(&translationValues, translationVector);

        const XMMATRIX rotationMatrix =
            XMMatrixRotationQuaternion(rotationQuaternion);
        XMFLOAT4X4 rotationValues{};
        XMStoreFloat4x4(&rotationValues, rotationMatrix);

        float pitchSine = -rotationValues._32;
        if (pitchSine > 1.0f) pitchSine = 1.0f;
        if (pitchSine < -1.0f) pitchSine = -1.0f;

        const float pitch = asinf(pitchSine);
        const float pitchCosine = cosf(pitch);

        float yaw = 0.0f;
        float roll = 0.0f;

        if (fabsf(pitchCosine) > 0.00001f) {
            roll = atan2f(rotationValues._12, rotationValues._22);
            yaw = atan2f(rotationValues._31, rotationValues._33);
        }
        else {
            // En bloqueo de cardan se fija roll en cero y se conserva la
            // orientacion restante dentro de yaw.
            yaw = atan2f(-rotationValues._13, rotationValues._11);
            roll = 0.0f;
        }

        position = EU::Vector3(
            translationValues.x,
            translationValues.y,
            translationValues.z);
        rotation = EU::Vector3(pitch, yaw, roll);
        scale = sanitizeScale(EU::Vector3(
            scaleValues.x,
            scaleValues.y,
            scaleValues.z));

        matrix = localMatrix;
        m_dirty = false;
        return true;
    }

    void translate(const EU::Vector3& translation) {
        position += translation;
        markDirty();
    }

    void rotate(const EU::Vector3& rotationDelta) {
        rotation += rotationDelta;
        markDirty();
    }

    void scaleBy(const EU::Vector3& scaleMultiplier) {
        scale.x *= scaleMultiplier.x;
        scale.y *= scaleMultiplier.y;
        scale.z *= scaleMultiplier.z;
        scale = sanitizeScale(scale);
        markDirty();
    }

    void reset() {
        position.zero();
        rotation.zero();
        scale.one();
        markDirty();
    }

    void markDirty() { m_dirty = true; }
    bool isDirty() const { return m_dirty; }

    void rebuildLocalMatrix() {
        if (!m_dirty) {
            return;
        }

        const XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);
        const XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(
            rotation.x, rotation.y, rotation.z);
        const XMMATRIX translationMatrix = XMMatrixTranslation(
            position.x, position.y, position.z);

        matrix = scaleMatrix * rotationMatrix * translationMatrix;
        m_dirty = false;
    }

    EU::Vector3 getForwardVector() const {
        return transformDirection(EU::Vector3(0.0f, 0.0f, 1.0f));
    }

    EU::Vector3 getRightVector() const {
        return transformDirection(EU::Vector3(1.0f, 0.0f, 0.0f));
    }

    EU::Vector3 getUpVector() const {
        return transformDirection(EU::Vector3(0.0f, 1.0f, 0.0f));
    }

    XMMATRIX matrix;
    XMMATRIX worldMatrix;

private:
    static EU::Vector3 sanitizeScale(const EU::Vector3& value) {
        const float minimumScale = 0.0001f;
        EU::Vector3 result = value;

        if (EU::abs(result.x) < minimumScale) {
            result.x = result.x < 0.0f ? -minimumScale : minimumScale;
        }
        if (EU::abs(result.y) < minimumScale) {
            result.y = result.y < 0.0f ? -minimumScale : minimumScale;
        }
        if (EU::abs(result.z) < minimumScale) {
            result.z = result.z < 0.0f ? -minimumScale : minimumScale;
        }

        return result;
    }

    EU::Vector3 transformDirection(const EU::Vector3& localDirection) const {
        const XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(
            rotation.x, rotation.y, rotation.z);
        const XMVECTOR direction = XMVector3TransformNormal(
            XMVectorSet(localDirection.x, localDirection.y, localDirection.z, 0.0f),
            rotationMatrix);
        const XMVECTOR normalized = XMVector3Normalize(direction);

        return EU::Vector3(
            XMVectorGetX(normalized),
            XMVectorGetY(normalized),
            XMVectorGetZ(normalized));
    }

private:
    EU::Vector3 position;
    EU::Vector3 rotation;
    EU::Vector3 scale;
    bool m_dirty;
};
