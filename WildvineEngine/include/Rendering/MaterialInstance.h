/**
 * @file MaterialInstance.h
 * @brief Declara la API de MaterialInstance dentro del subsistema Rendering.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class Material;
class DeviceContext;
class Texture;

/**
 * @class MaterialInstance
 * @brief Agrupa un material base con sus texturas y parametros concretos.
 *
 * Esta clase permite reutilizar un mismo `Material` con diferentes mapas de texturas
 * y parametros PBR por objeto renderizado.
 */
class MaterialInstance {
public:
	/**
	 * @brief Asigna el material base que define el comportamiento del shader y el estado del pipeline.
	 * @param material Puntero al `Material` a instanciar.
	 */
	void setMaterial(Material* material) { m_material = material; }

	/**
	 * @brief Asigna la textura de color base (Albedo/Diffuse).
	 * @param texture Puntero a la textura de Albedo.
	 */
	void setAlbedo(Texture* texture) { m_albedo = texture; }

	/**
	 * @brief Asigna el mapa de normales (Normal Map) para el detalle de relieve de la superficie.
	 * @param texture Puntero a la textura de Normales.
	 */
	void setNormal(Texture* texture) { m_normal = texture; }

	/**
	 * @brief Asigna el mapa de metalizado (Metallic) para propiedades de renderizado basado en física (PBR).
	 * @param texture Puntero a la textura de Metalizado.
	 */
	void setMetallic(Texture* texture) { m_metallic = texture; }

	/**
	 * @brief Asigna el mapa de rugosidad (Roughness) para definir la dispersión especular de la superficie.
	 * @param texture Puntero a la textura de Rugosidad.
	 */
	void setRoughness(Texture* texture) { m_roughness = texture; }

	/**
	 * @brief Asigna el mapa de oclusión ambiental (Ambient Occlusion / AO).
	 * @param texture Puntero a la textura de AO.
	 */
	void setAO(Texture* texture) { m_ao = texture; }

	/**
	 * @brief Asigna el mapa de emisión (Emissive) para superficies que proyectan luz propia.
	 * @param texture Puntero a la textura Emisiva.
	 */
	void setEmissive(Texture* texture) { m_emissive = texture; }

	/**
	 * @brief Obtiene el material base asociado a esta instancia.
	 * @return Puntero al `Material` actual.
	 */
	Material* getMaterial() const { return m_material; }

	/**
	 * @brief Obtiene la textura de color base (Albedo) asignada.
	 * @return Puntero a la textura de Albedo.
	 */
	Texture* getAlbedo() const { return m_albedo; }

	/**
	 * @brief Obtiene el mapa de normales asignado.
	 * @return Puntero a la textura de Normales.
	 */
	Texture* getNormal() const { return m_normal; }

	/**
	 * @brief Obtiene el mapa de metalizado asignado.
	 * @return Puntero a la textura de Metalizado.
	 */
	Texture* getMetallic() const { return m_metallic; }

	/**
	 * @brief Obtiene el mapa de rugosidad asignado.
	 * @return Puntero a la textura de Rugosidad.
	 */
	Texture* getRoughness() const { return m_roughness; }

	/**
	 * @brief Obtiene el mapa de oclusión ambiental asignado.
	 * @return Puntero a la textura de AO.
	 */
	Texture* getAO() const { return m_ao; }

	/**
	 * @brief Obtiene el mapa de emisión asignado.
	 * @return Puntero a la textura Emisiva.
	 */
	Texture* getEmissive() const { return m_emissive; }

	/**
	 * @brief Obtiene una referencia modificable a los parámetros escalares o vectoriales del material.
	 * @return Referencia a la estructura `MaterialParams`.
	 */
	MaterialParams& getParams() { return m_params; }

	/**
	 * @brief Obtiene una referencia de solo lectura a los parámetros escalares o vectoriales del material.
	 * @return Referencia constante a la estructura `MaterialParams`.
	 */
	const MaterialParams& getParams() const { return m_params; }

	/**
	 * @brief Enlaza las texturas de la instancia en el contexto grafico actual.
	 * @param deviceContext Contexto del dispositivo utilizado para enlazar (bind) los recursos al pipeline.
	 */
	void bindTextures(DeviceContext& deviceContext) const;

private:
	Material* m_material = nullptr;      ///< Puntero al material base (estado del pipeline y shaders).
	Texture* m_albedo = nullptr;         ///< Mapa de color base o difuso (RGB).
	Texture* m_normal = nullptr;         ///< Mapa de normales en espacio tangente (XYZ).
	Texture* m_metallic = nullptr;       ///< Mapa que define qué tan metálica es la superficie (Escala de grises).
	Texture* m_roughness = nullptr;      ///< Mapa que define la micro-rugosidad de la superficie (Escala de grises).
	Texture* m_ao = nullptr;             ///< Mapa de oclusión ambiental para representar micro-sombras (Escala de grises).
	Texture* m_emissive = nullptr;       ///< Mapa de emisión de luz propia (RGB).

	MaterialParams m_params;             ///< Estructura con parámetros PBR base (como multiplicadores) para el constant buffer.
};