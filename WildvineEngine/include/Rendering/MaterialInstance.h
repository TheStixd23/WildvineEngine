/**
 * @file MaterialInstance.h
 * @brief Declara la API de MaterialInstance dentro del subsistema Rendering de WildvineEngine.
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
class
	MaterialInstance {
public:
	/**
	 * @brief Asigna el material base a esta instancia.
	 * @param material Puntero al material base.
	 */
	void setMaterial(Material* material) { m_material = material; }

	/**
	 * @brief Asigna la textura de color base (Albedo).
	 * @param texture Puntero a la textura de Albedo.
	 */
	void setAlbedo(Texture* texture) { m_albedo = texture; }

	/**
	 * @brief Asigna la textura de mapa de normales.
	 * @param texture Puntero a la textura de normales.
	 */
	void setNormal(Texture* texture) { m_normal = texture; }

	/**
	 * @brief Asigna la textura de metalicidad.
	 * @param texture Puntero a la textura metálica.
	 */
	void setMetallic(Texture* texture) { m_metallic = texture; }

	/**
	 * @brief Asigna la textura de rugosidad (Roughness).
	 * @param texture Puntero a la textura de rugosidad.
	 */
	void setRoughness(Texture* texture) { m_roughness = texture; }

	/**
	 * @brief Asigna la textura de oclusión ambiental (AO).
	 * @param texture Puntero a la textura de AO.
	 */
	void setAO(Texture* texture) { m_ao = texture; }

	/**
	 * @brief Asigna la textura de emisión de luz.
	 * @param texture Puntero a la textura emisiva.
	 */
	void setEmissive(Texture* texture) { m_emissive = texture; }

	/**
	 * @brief Obtiene el material base asociado.
	 * @return Material* Puntero al material.
	 */
	Material* getMaterial() const { return m_material; }

	/**
	 * @brief Obtiene la textura de color base (Albedo).
	 * @return Texture* Puntero a la textura de Albedo.
	 */
	Texture* getAlbedo() const { return m_albedo; }

	/**
	 * @brief Obtiene la textura de normales.
	 * @return Texture* Puntero a la textura de normales.
	 */
	Texture* getNormal() const { return m_normal; }

	/**
	 * @brief Obtiene la textura de metalicidad.
	 * @return Texture* Puntero a la textura metálica.
	 */
	Texture* getMetallic() const { return m_metallic; }

	/**
	 * @brief Obtiene la textura de rugosidad (Roughness).
	 * @return Texture* Puntero a la textura de rugosidad.
	 */
	Texture* getRoughness() const { return m_roughness; }

	/**
	 * @brief Obtiene la textura de oclusión ambiental (AO).
	 * @return Texture* Puntero a la textura de AO.
	 */
	Texture* getAO() const { return m_ao; }

	/**
	 * @brief Obtiene la textura emisiva.
	 * @return Texture* Puntero a la textura emisiva.
	 */
	Texture* getEmissive() const { return m_emissive; }

	/**
	 * @brief Obtiene una referencia modificable a los parámetros numéricos del material.
	 * @return MaterialParams& Referencia a la estructura de parámetros PBR.
	 */
	MaterialParams& getParams() { return m_params; }

	/**
	 * @brief Obtiene una referencia de solo lectura a los parámetros numéricos del material.
	 * @return const MaterialParams& Referencia constante a la estructura de parámetros PBR.
	 */
	const MaterialParams& getParams() const { return m_params; }

	/**
	 * @brief Enlaza las texturas de la instancia en el contexto grafico actual.
	 * @param deviceContext Contexto del dispositivo gráfico usado para emitir los comandos de enlace.
	 */
	void bindTextures(DeviceContext& deviceContext) const;

private:
	Material* m_material = nullptr;      ///< Puntero al material base compartido.
	Texture* m_albedo = nullptr;         ///< Textura de color base (Albedo).
	Texture* m_normal = nullptr;         ///< Textura de normales.
	Texture* m_metallic = nullptr;       ///< Textura de metalicidad.
	Texture* m_roughness = nullptr;      ///< Textura de rugosidad (Roughness).
	Texture* m_ao = nullptr;             ///< Textura de oclusión ambiental (Ambient Occlusion).
	Texture* m_emissive = nullptr;       ///< Textura emisiva.
	MaterialParams m_params;             ///< Parámetros numéricos específicos de esta instancia.
};