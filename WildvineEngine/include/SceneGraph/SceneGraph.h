/**
 * @file SceneGraph.h
 * @brief Declara la API de SceneGraph dentro del subsistema SceneGraph de WildvineEngine.
 * @ingroup scenegraph
 */
#pragma once
#include "Prerequisites.h"

class Entity;
class DeviceContext;
class Camera;
class RenderScene;

/**
 * @class SceneGraph
 * @brief Administra la jerarquía de entidades y su actualización espacial.
 *
 * El `SceneGraph` actúa como el sistema central para registrar entidades, resolver
 * relaciones padre-hijo (jerarquías locales vs globales) y propagar transformaciones
 * espaciales (World Matrices) antes de empaquetar la información geométrica para
 * el pipeline de renderizado.
 */
class
	SceneGraph {
public:
	/**
	 * @brief Constructor por defecto.
	 */
	SceneGraph() = default;

	/**
	 * @brief Destructor por defecto.
	 */
	~SceneGraph() = default;

	/**
	 * @brief Inicializa las estructuras base del grafo de escena.
	 */
	void
		init();

	/**
	 * @brief Registra una entidad independiente dentro del grafo.
	 * @param e Puntero a la entidad a registrar.
	 */
	void
		addEntity(Entity* e);

	/**
	 * @brief Elimina de manera segura una entidad del grafo si estaba registrada.
	 * @param e Puntero a la entidad a retirar.
	 */
	void
		removeEntity(Entity* e);

	/**
	 * @brief Comprueba recursivamente si un nodo es ascendiente de otro (para evitar ciclos).
	 * @param possibleAncestor Nodo que se sospecha es padre, abuelo, etc.
	 * @param node Nodo de referencia a evaluar.
	 * @return true Si `possibleAncestor` está en la cadena jerárquica hacia la raíz de `node`.
	 */
	bool
		isAncestor(Entity* possibleAncestor, Entity* node) const;

	/**
	 * @brief Vincula una entidad como hija de otra, estableciendo una relación local.
	 * @param child Entidad que adoptará transformaciones relativas.
	 * @param parent Entidad que actuará como sistema de coordenadas de origen.
	 * @return true Si el enganche fue exitoso (no hubo ciclos de dependencia).
	 */
	bool
		attach(Entity* child, Entity* parent);

	/**
	 * @brief Desvincula una entidad de su padre, volviéndola independiente (raíz local).
	 * @param child Entidad a separar.
	 * @return true Si la desvinculación se completó correctamente.
	 */
	bool
		detach(Entity* child);

	/**
	 * @brief Recorre el grafo propagando transformaciones y actualizando lógica de componentes.
	 * @param deltaTime Tiempo transcurrido en segundos desde la última llamada.
	 * @param deviceContext Contexto del dispositivo para actualizaciones relacionadas con GPU.
	 */
	void
		update(float deltaTime, DeviceContext& deviceContext);

	/**
	 * @brief Función de soporte para dibujar gizmos o visualizar la jerarquía en depuración.
	 * @param deviceContext Contexto del dispositivo gráfico.
	 */
	void
		render(DeviceContext& deviceContext);

	/**
	 * @brief Extrae los objetos renderizables del grafo y los clasifica en una `RenderScene`.
	 * @param outScene Estructura que será poblada con mallas, luces y cámaras activas.
	 * @param camera Cámara principal que se usa de referencia (ej. para calcular distancias o frustum culling).
	 */
	void
		gatherRenderScene(RenderScene& outScene, const Camera& camera);

	/**
	 * @brief Limpia la jerarquía y vacía todas las referencias a entidades.
	 */
	void
		destroy();

private:
	/**
	 * @brief Propaga matemáticamente la matriz acumulada a través de la jerarquía descendente.
	 * @param node Nodo actual a actualizar.
	 * @param parentWorld Matriz combinada proveniente de su antecesor inmediato.
	 */
	void
		updateWorldRecursive(Entity* node, const XMMATRIX& parentWorld);

	/**
	 * @brief Verifica si una entidad está en el nivel superior (sin padre).
	 * @param e Entidad a evaluar.
	 * @return true Si la entidad no tiene antecesores.
	 */
	bool
		isRoot(Entity* e) const;

	/**
	 * @brief Verifica si el grafo de escena tiene conocimiento o control de la entidad dada.
	 * @param e Entidad a evaluar.
	 * @return true Si la entidad pertenece a `m_entities`.
	 */
	bool
		isRegistered(Entity* e) const;

public:
	std::vector<Entity*> m_entities; ///< Colección lineal de todas las entidades registradas y gestionadas por este grafo.
};