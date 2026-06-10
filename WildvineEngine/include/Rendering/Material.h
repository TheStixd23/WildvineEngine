/**
 * @file Material.h
 * @brief Declara la API de Material dentro del subsistema Rendering de WildvineEngine.
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
class
	Material {
public:
	/**
	 * @brief Asigna el programa de shader principal que utilizará el material.
	 * @param shader Puntero al programa de shader.
	 */
	void
		setShader(ShaderProgram* shader) { m_shader = shader; }

	/**
	 * @brief Define el estado de rasterización para las superficies que usen este material.
	 * @param state Puntero al estado de rasterización.
	 */
	void
		setRasterizerState(RasterizerState* state) { m_rasterizerState = state; }

	/**
	 * @brief Define el estado de prueba de profundidad y esténcil.
	 * @param state Puntero al estado de profundidad/esténcil.
	 */
	void
		setDepthStencilState(DepthStencilState* state) { m_depthStencilState = state; }

	/**
	 * @brief Configura el sampler por defecto que se utilizará para muestrear las texturas.
	 * @param state Puntero al estado del sampler.
	 */
	void
		setSamplerState(SamplerState* state) { m_samplerState = state; }

	/**
	 * @brief Establece el dominio de renderizado (ej. Opaco, Transparente).
	 * @param domain Dominio al que pertenece el material.
	 */
	void
		setDomain(MaterialDomain domain) { m_domain = domain; }

	/**
	 * @brief Establece el modo de mezcla a utilizar (ej. Opaque, Alpha, Additive).
	 * @param blendMode Modo de mezcla solicitado.
	 */
	void
		setBlendMode(BlendMode blendMode) { m_blendMode = blendMode; }

	/**
	 * @brief Obtiene el shader principal asociado al material.
	 * @return ShaderProgram* Puntero al shader, o nullptr si no ha sido asignado.
	 */
	ShaderProgram* getShader() const { return m_shader; }

	/**
	 * @brief Obtiene el estado de rasterización asociado.
	 * @return RasterizerState* Puntero al estado de rasterización.
	 */
	RasterizerState* getRasterizerState() const { return m_rasterizerState; }

	/**
	 * @brief Obtiene el estado de profundidad y esténcil asociado.
	 * @return DepthStencilState* Puntero al estado de profundidad/esténcil.
	 */
	DepthStencilState* getDepthStencilState() const { return m_depthStencilState; }

	/**
	 * @brief Obtiene el sampler por defecto de las texturas.
	 * @return SamplerState* Puntero al estado del sampler.
	 */
	SamplerState* getSamplerState() const { return m_samplerState; }

	/**
	 * @brief Obtiene el dominio de renderizado al que pertenece el material.
	 * @return MaterialDomain Dominio actual del material.
	 */
	MaterialDomain getDomain() const { return m_domain; }

	/**
	 * @brief Obtiene el modo de mezcla configurado en el material.
	 * @return BlendMode Modo de mezcla actual.
	 */
	BlendMode getBlendMode() const { return m_blendMode; }

private:
	ShaderProgram* m_shader = nullptr;                   ///< Shader principal del material.
	RasterizerState* m_rasterizerState = nullptr;        ///< Estado de rasterizacion asociado.
	DepthStencilState* m_depthStencilState = nullptr;    ///< Estado de profundidad/estencil asociado.
	SamplerState* m_samplerState = nullptr;              ///< Sampler por defecto para texturas del material.
	MaterialDomain m_domain = MaterialDomain::Opaque;    ///< Dominio de render del material.
	BlendMode m_blendMode = BlendMode::Opaque;           ///< Modo de mezcla solicitado por el material.
};