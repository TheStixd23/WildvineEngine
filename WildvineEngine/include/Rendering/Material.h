/**
 * @file Material.h
 * @brief Declara la API de Material dentro del subsistema Rendering.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class ShaderProgram;
class RasterizerState;
class DepthStencilState;
class SamplerState;

/**
 * @class Material
 * @brief Describe el estado fijo compartido por una o mas instancias de material.
 *
 * Un `Material` apunta a shader, estados de rasterizacion/profundidad y al modo de
 * mezcla que debe aplicar el renderer al dibujar una superficie.
 */
class Material {
public:
	/**
	 * @brief Asigna el programa de shaders principal que utilizará este material.
	 * @param shader Puntero al `ShaderProgram` a asociar.
	 */
	void setShader(ShaderProgram* shader) { m_shader = shader; }

	/**
	 * @brief Asigna el estado del rasterizador para este material.
	 * @param state Puntero al `RasterizerState` que define propiedades como el culling o wireframe.
	 */
	void setRasterizerState(RasterizerState* state) { m_rasterizerState = state; }

	/**
	 * @brief Asigna el estado de profundidad y stencil (Depth/Stencil).
	 * @param state Puntero al `DepthStencilState` que controla la escritura y test de profundidad.
	 */
	void setDepthStencilState(DepthStencilState* state) { m_depthStencilState = state; }

	/**
	 * @brief Asigna el estado del sampler por defecto para las texturas del material.
	 * @param state Puntero al `SamplerState` que define el filtrado y el modo de envoltura (wrap/clamp).
	 */
	void setSamplerState(SamplerState* state) { m_samplerState = state; }

	/**
	 * @brief Establece el dominio geométrico de renderizado del material (ej. Opaco, Transparente).
	 * @param domain Valor de la enumeración `MaterialDomain`.
	 */
	void setDomain(MaterialDomain domain) { m_domain = domain; }

	/**
	 * @brief Establece el modo de mezcla (Blending) que utilizará este material al dibujarse.
	 * @param blendMode Valor de la enumeración `BlendMode`.
	 */
	void setBlendMode(BlendMode blendMode) { m_blendMode = blendMode; }

	/**
	 * @brief Obtiene el programa de shaders asociado a este material.
	 * @return Puntero al `ShaderProgram` actual.
	 */
	ShaderProgram* getShader() const { return m_shader; }

	/**
	 * @brief Obtiene el estado del rasterizador del material.
	 * @return Puntero al `RasterizerState` actual.
	 */
	RasterizerState* getRasterizerState() const { return m_rasterizerState; }

	/**
	 * @brief Obtiene el estado de profundidad y stencil del material.
	 * @return Puntero al `DepthStencilState` actual.
	 */
	DepthStencilState* getDepthStencilState() const { return m_depthStencilState; }

	/**
	 * @brief Obtiene el estado del sampler del material.
	 * @return Puntero al `SamplerState` actual.
	 */
	SamplerState* getSamplerState() const { return m_samplerState; }

	/**
	 * @brief Obtiene el dominio de renderizado configurado.
	 * @return El valor actual de `MaterialDomain`.
	 */
	MaterialDomain getDomain() const { return m_domain; }

	/**
	 * @brief Obtiene el modo de mezcla configurado.
	 * @return El valor actual de `BlendMode`.
	 */
	BlendMode getBlendMode() const { return m_blendMode; }

private:
	ShaderProgram* m_shader = nullptr;                 ///< Shader principal del material.
	RasterizerState* m_rasterizerState = nullptr;      ///< Estado de rasterizacion asociado.
	DepthStencilState* m_depthStencilState = nullptr;  ///< Estado de profundidad/estencil asociado.
	SamplerState* m_samplerState = nullptr;            ///< Sampler por defecto para texturas del material.
	MaterialDomain m_domain = MaterialDomain::Opaque;  ///< Dominio de render del material.
	BlendMode m_blendMode = BlendMode::Opaque;         ///< Modo de mezcla solicitado por el material.
};