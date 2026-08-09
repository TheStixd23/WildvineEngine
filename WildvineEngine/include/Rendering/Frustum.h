#pragma once

#include "Prerequisites.h"
#include <array>

/** Resultado detallado de la prueba AABB contra el frustum. */
enum class FrustumBoxResult {
    Invalid = 0,
    Outside,
    Intersecting,
    Inside
};

/**
 * @class Frustum
 * @brief Volumen visible de la camara usado para Frustum Culling.
 *
 * La prueba es conservadora: transforma las 8 esquinas de una AABB local
 * hasta espacio clip de Direct3D. Un objeto solo se descarta si las ocho
 * esquinas quedan completamente fuera del mismo plano del frustum.
 *
 * Ante matrices, bounds o coordenadas invalidas el objeto se considera
 * visible. De esta manera un error de datos no provoca que un modelo
 * desaparezca accidentalmente del editor.
 */
class Frustum {
public:
    Frustum();

    /** Actualiza el frustum con las matrices actuales de la camara. */
    void update(const XMMATRIX& view, const XMMATRIX& projection);

    /**
     * @return true si la caja puede intersectar el volumen visible.
     */
    bool isBoxVisible(
        const EU::Vector3& localMinimum,
        const EU::Vector3& localMaximum,
        const XMMATRIX& world) const;

    /** Devuelve Outside, Intersecting, Inside o Invalid para debug. */
    FrustumBoxResult classifyBox(
        const EU::Vector3& localMinimum,
        const EU::Vector3& localMaximum,
        const XMMATRIX& world) const;

    /** Obtiene las ocho esquinas del frustum en espacio mundo. */
    bool getWorldCorners(std::array<EU::Vector3, 8>& outCorners) const;

    bool isValid() const { return m_valid; }

private:
    XMFLOAT4X4 m_viewProjection{};
    std::array<EU::Vector3, 8> m_worldCorners{};
    bool m_valid = false;
    bool m_hasWorldCorners = false;
};
