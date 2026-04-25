/**
 * @file RenderScene.h
 * @brief Declara la API de RenderScene dentro del subsistema Rendering de MonacoEngine3.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class Skybox;

/**
 * @class RenderScene
 * @brief Contenedor temporal con los elementos visibles de un frame para MonacoEngine3.
 *
 * `RenderScene` funciona como estructura intermedia entre el `SceneGraph` y el
 * renderer. Agrupa objetos por tipo de cola (opacos o transparentes), almacena
 * los datos de luces direccionales y mantiene la referencia al skybox activo.
 */
class RenderScene {
public:
	/**
	 * @brief Limpia todas las colecciones para preparar un nuevo frame.
	 * * Vacía los vectores de geometría y resetea los punteros de entorno (como el skybox)
	 * antes de iniciar la fase de recolección (culling) del siguiente ciclo de renderizado.
	 */
	void clear();

public:
	std::vector<RenderObject> opaqueObjects;       ///< Lista de objetos opacos listos para renderizar. Usualmente se benefician de ordenamiento por material u optimizaciones de Z-buffer.
	std::vector<RenderObject> transparentObjects;  ///< Lista de objetos con transparencia. Deben ser ordenados por profundidad (de atrás hacia adelante) antes de despachar el dibujo.
	std::vector<LightData> directionalLights;      ///< Colección de datos estructurales de las luces direccionales activas que iluminarán la escena.
	Skybox* skybox = nullptr;                      ///< Puntero al entorno mapeado (Skybox) activo para el frame actual.
};