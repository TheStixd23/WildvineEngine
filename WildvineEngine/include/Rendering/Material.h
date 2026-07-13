#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

// Forward declarations para optimizar tiempos de compilación
class ShaderProgram;
class RasterizerState;
class DepthStencilState;
class SamplerState;

/**
 * @class Material
 * @brief Representa un material de renderizado que encapsula los estados de la GPU y los shaders.
 *
 * La clase Material agrupa la combinación de shaders (programas) y configuraciones del pipeline
 * (rasterizado, pruebas de profundidad/stencil, muestreo de texturas y modos de mezcla)
 * necesarios para definir la apariencia visual de un objeto en la escena.
 */
class Material {
public:
    /** @name Setters (Configuración del Material) */
    ///@{

    /**
     * @brief Asigna el programa de shaders que utilizará este material.
     * @param shader Puntero al ShaderProgram (Vertex, Pixel, etc.).
     */
    void setShader(ShaderProgram* shader) { m_shader = shader; }

    /**
     * @brief Define el estado del rasterizador (e.g., Culling, Wireframe).
     * @param state Puntero al objeto que contiene la configuración de rasterización.
     */
    void setRasterizerState(RasterizerState* state) { m_rasterizerState = state; }

    /**
     * @brief Define el estado de la prueba de profundidad y stencil.
     * @param state Puntero con la configuración de profundidad y máscara de stencil.
     */
    void setDepthStencilState(DepthStencilState* state) { m_depthStencilState = state; }

    /**
     * @brief Asigna el estado del muestreador de texturas (e.g., filtrado Anisotrópico, Bilineal).
     * @param state Puntero con la configuración de muestreo de texturas.
     */
    void setSamplerState(SamplerState* state) { m_samplerState = state; }

    /**
     * @brief Establece el dominio del material (e.g., Opaco, Translúcido, Post-Procesado).
     * @param domain Tipo de dominio proveniente de MaterialDomain.
     */
    void setDomain(MaterialDomain domain) { m_domain = domain; }

    /**
     * @brief Configura el modo de mezcla (Blend Mode) para la combinación de colores en el render target.
     * @param blendMode Tipo de mezcla proveniente de BlendMode.
     */
    void setBlendMode(BlendMode blendMode) { m_blendMode = blendMode; }
    ///@}

    /** @name Getters (Consulta de Propiedades) */
    ///@{

    /** @return ShaderProgram* Puntero al shader actual asignado. */
    ShaderProgram* getShader() const { return m_shader; }

    /** @return RasterizerState* Puntero al estado del rasterizador actual. */
    RasterizerState* getRasterizerState() const { return m_rasterizerState; }

    /** @return DepthStencilState* Puntero al estado de profundidad y stencil actual. */
    DepthStencilState* getDepthStencilState() const { return m_depthStencilState; }

    /** @return SamplerState* Puntero al estado del muestreador actual. */
    SamplerState* getSamplerState() const { return m_samplerState; }

    /** @return MaterialDomain El dominio actual al que pertenece este material. */
    MaterialDomain getDomain() const { return m_domain; }

    /** @return BlendMode El modo de mezcla actual del material. */
    BlendMode getBlendMode() const { return m_blendMode; }
    ///@}

private:
    ShaderProgram* m_shader = nullptr;             ///< Programa de shaders activo.
    RasterizerState* m_rasterizerState = nullptr;   ///< Configuración del pipeline de rasterización.
    DepthStencilState* m_depthStencilState = nullptr; ///< Configuración de pruebas Z-buffer y Stencil.
    SamplerState* m_samplerState = nullptr;         ///< Configuración de filtrado y envoltura de texturas.

    MaterialDomain m_domain = MaterialDomain::Opaque; ///< Dominio del material. Por defecto: Opaco.
    BlendMode m_blendMode = BlendMode::Opaque;       ///< Modo de mezcla. Por defecto: Opaco.
};