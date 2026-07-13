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
 * @brief Agrupa un material base con sus texturas y parámetros concretos.
 *
 * Esta clase implementa el patrón de instanciación de materiales. Permite reutilizar
 * un mismo 'Material' (que comparte shaders y estados de la GPU) con diferentes
 * mapas de texturas PBR (Physically Based Rendering) y parámetros numéricos por cada objeto.
 */
class MaterialInstance {
public:
    /** @name Setters (Configuración de Recursos) */
    ///@{

    /**
     * @brief Asigna el material base del cual heredará los shaders y estados de renderizado.
     * @param material Puntero al Material base.
     */
    void setMaterial(Material* material) { m_material = material; }

    /**
     * @brief Asigna el mapa de Albedo (Color base / Difuso).
     * @param texture Puntero a la textura de color.
     */
    void setAlbedo(Texture* texture) { m_albedo = texture; }

    /**
     * @brief Asigna el mapa de Normales (Detalle de relieve en la superficie).
     * @param texture Puntero a la textura de normales (Normal Map).
     */
    void setNormal(Texture* texture) { m_normal = texture; }

    /**
     * @brief Asigna el mapa de Metalicidad (Propiedades metálicas del material).
     * @param texture Puntero a la textura de escala de grises para metalicidad.
     */
    void setMetallic(Texture* texture) { m_metallic = texture; }

    /**
     * @brief Asigna el mapa de Rugosidad / Microfacetas (Roughness).
     * @param texture Puntero a la textura de escala de grises para rugosidad.
     */
    void setRoughness(Texture* texture) { m_roughness = texture; }

    /**
     * @brief Asigna el mapa de Oclusión Ambiental (Ambient Occlusion).
     * @param texture Puntero a la textura de sombreado estático por contacto.
     */
    void setAO(Texture* texture) { m_ao = texture; }

    /**
     * @brief Asigna el mapa de Emisión (Luz propia que emite el material).
     * @param texture Puntero a la textura de emisión de color.
     */
    void setEmissive(Texture* texture) { m_emissive = texture; }
    ///@}

    /** @name Getters (Consulta de Recursos y Parámetros) */
    ///@{

    /** @return Material* Puntero al material maestro base. */
    Material* getMaterial() const { return m_material; }

    /** @return Texture* Puntero a la textura de Albedo activa. */
    Texture* getAlbedo() const { return m_albedo; }

    /** @return Texture* Puntero al mapa de normales activo. */
    Texture* getNormal() const { return m_normal; }

    /** @return Texture* Puntero al mapa de metalicidad activo. */
    Texture* getMetallic() const { return m_metallic; }

    /** @return Texture* Puntero al mapa de rugosidad activo. */
    Texture* getRoughness() const { return m_roughness; }

    /** @return Texture* Puntero al mapa de oclusión ambiental activo. */
    Texture* getAO() const { return m_ao; }

    /** @return Texture* Puntero al mapa de emisión activo. */
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
    ///@}

    /**
     * @brief Enlaza las texturas de la instancia en el contexto gráfico actual.
     * * Este método se invoca antes de la llamada de dibujo (Draw Call) para enviar las
     * texturas asignadas a los *slots* correspondientes de la GPU a través del pipeline.
     * * @param deviceContext Referencia al contexto de la API gráfica (e.g., DirectX/Vulkan).
     */
    void bindTextures(DeviceContext& deviceContext) const;

private:
    Material* m_material = nullptr;      ///< Material base (Shaders y Pipeline States).
    Texture* m_albedo = nullptr;        ///< Textura de color base (RGB).
    Texture* m_normal = nullptr;        ///< Mapa de normales del espacio de tangente (RGB).
    Texture* m_metallic = nullptr;      ///< Mapa de metalicidad (Escala de grises).
    Texture* m_roughness = nullptr;     ///< Mapa de rugosidad (Escala de grises).
    Texture* m_ao = nullptr;            ///< Mapa de oclusión ambiental (Escala de grises).
    Texture* m_emissive = nullptr;      ///< Mapa de auto-iluminación (RGB).
    MaterialParams m_params;            ///< Contenedor de propiedades numéricas del material (e.g., escalares, vectores).
};