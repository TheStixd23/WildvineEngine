#include "Rendering/Frustum.h"

#include <cmath>

namespace {
    bool isFiniteVector4(const XMFLOAT4& value) {
        return std::isfinite(value.x) &&
            std::isfinite(value.y) &&
            std::isfinite(value.z) &&
            std::isfinite(value.w);
    }

    bool hasValidBounds(
        const EU::Vector3& minimum,
        const EU::Vector3& maximum) {

        if (!std::isfinite(minimum.x) ||
            !std::isfinite(minimum.y) ||
            !std::isfinite(minimum.z) ||
            !std::isfinite(maximum.x) ||
            !std::isfinite(maximum.y) ||
            !std::isfinite(maximum.z)) {
            return false;
        }

        return minimum.x <= maximum.x &&
            minimum.y <= maximum.y &&
            minimum.z <= maximum.z;
    }

    bool isFiniteMatrix(const XMFLOAT4X4& matrix) {
        const float* values = &matrix._11;
        for (int index = 0; index < 16; ++index) {
            if (!std::isfinite(values[index])) {
                return false;
            }
        }
        return true;
    }
}

Frustum::Frustum() {
    XMStoreFloat4x4(
        &m_viewProjection,
        XMMatrixIdentity());
}

void Frustum::update(
    const XMMATRIX& view,
    const XMMATRIX& projection) {

    const XMMATRIX viewProjection = view * projection;
    XMFLOAT4X4 stored{};
    XMStoreFloat4x4(&stored, viewProjection);

    if (!isFiniteMatrix(stored)) {
        m_valid = false;
        m_hasWorldCorners = false;
        return;
    }

    m_viewProjection = stored;
    m_valid = true;
    m_hasWorldCorners = false;

    XMVECTOR determinant = XMVectorZero();
    const XMMATRIX inverseViewProjection =
        XMMatrixInverse(&determinant, viewProjection);

    const float determinantValue = XMVectorGetX(determinant);
    if (!std::isfinite(determinantValue) ||
        fabsf(determinantValue) <= 1e-8f) {
        return;
    }

    // Direct3D NDC: X/Y [-1,1], Z [0,1].
    for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
        const float x = (cornerIndex & 1) ? 1.0f : -1.0f;
        const float y = (cornerIndex & 2) ? 1.0f : -1.0f;
        const float z = (cornerIndex & 4) ? 1.0f : 0.0f;

        const XMVECTOR worldCorner = XMVector3TransformCoord(
            XMVectorSet(x, y, z, 1.0f),
            inverseViewProjection);

        XMFLOAT3 world{};
        XMStoreFloat3(&world, worldCorner);

        if (!std::isfinite(world.x) ||
            !std::isfinite(world.y) ||
            !std::isfinite(world.z)) {
            m_hasWorldCorners = false;
            return;
        }

        m_worldCorners[cornerIndex] =
            EU::Vector3(world.x, world.y, world.z);
    }

    m_hasWorldCorners = true;
}

FrustumBoxResult Frustum::classifyBox(
    const EU::Vector3& localMinimum,
    const EU::Vector3& localMaximum,
    const XMMATRIX& world) const {

    if (!m_valid ||
        !hasValidBounds(localMinimum, localMaximum)) {
        return FrustumBoxResult::Invalid;
    }

    const XMMATRIX viewProjection =
        XMLoadFloat4x4(&m_viewProjection);
    const XMMATRIX worldViewProjection =
        world * viewProjection;

    // Direct3D clip space (LH):
    // -W <= X <= W
    // -W <= Y <= W
    //  0 <= Z <= W
    bool allLeft = true;
    bool allRight = true;
    bool allBottom = true;
    bool allTop = true;
    bool allNear = true;
    bool allFar = true;
    bool allInside = true;

    for (int cornerIndex = 0;
        cornerIndex < 8;
        ++cornerIndex) {

        const float x = (cornerIndex & 1)
            ? localMaximum.x
            : localMinimum.x;
        const float y = (cornerIndex & 2)
            ? localMaximum.y
            : localMinimum.y;
        const float z = (cornerIndex & 4)
            ? localMaximum.z
            : localMinimum.z;

        const XMVECTOR localCorner =
            XMVectorSet(x, y, z, 1.0f);
        const XMVECTOR clipCorner =
            XMVector4Transform(
                localCorner,
                worldViewProjection);

        XMFLOAT4 clip{};
        XMStoreFloat4(&clip, clipCorner);

        if (!isFiniteVector4(clip)) {
            return FrustumBoxResult::Invalid;
        }

        const bool left = clip.x < -clip.w;
        const bool right = clip.x > clip.w;
        const bool bottom = clip.y < -clip.w;
        const bool top = clip.y > clip.w;
        const bool nearPlane = clip.z < 0.0f;
        const bool farPlane = clip.z > clip.w;

        allLeft = allLeft && left;
        allRight = allRight && right;
        allBottom = allBottom && bottom;
        allTop = allTop && top;
        allNear = allNear && nearPlane;
        allFar = allFar && farPlane;

        const bool inside =
            clip.w > 0.0001f &&
            !left &&
            !right &&
            !bottom &&
            !top &&
            !nearPlane &&
            !farPlane;

        allInside = allInside && inside;
    }

    if (allLeft ||
        allRight ||
        allBottom ||
        allTop ||
        allNear ||
        allFar) {
        return FrustumBoxResult::Outside;
    }

    return allInside
        ? FrustumBoxResult::Inside
        : FrustumBoxResult::Intersecting;
}

bool Frustum::isBoxVisible(
    const EU::Vector3& localMinimum,
    const EU::Vector3& localMaximum,
    const XMMATRIX& world) const {

    return classifyBox(
        localMinimum,
        localMaximum,
        world) != FrustumBoxResult::Outside;
}

bool Frustum::getWorldCorners(
    std::array<EU::Vector3, 8>& outCorners) const {

    if (!m_valid || !m_hasWorldCorners) {
        return false;
    }

    outCorners = m_worldCorners;
    return true;
}
