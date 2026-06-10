/**
 * @file HierarchyComponent.h
 * @brief Declara la API de HierarchyComponent dentro del subsistema SceneGraph de WildvineEngine.
 * @ingroup scenegraph
 */
#pragma once
#include "Prerequisites.h"
#include "ECS/Component.h"

class DeviceContext;
class Entity;

/**
 * @class HierarchyComponent
 * @brief Componente que gestiona las relaciones de parentesco en el grafo de escena.
 *
 * Permite a una entidad (Entity) actuar como un nodo dentro del Scene Graph,
 * manteniendo referencias directas a una entidad padre y múltiples entidades hijas
 * para la propagación de transformaciones o lógica jerárquica.
 */
class
	HierarchyComponent : public Component {
public:
	/**
	 * @brief Constructor por defecto del componente jerárquico.
	 */
	HierarchyComponent() : Component(ComponentType::HIERARCHY) {}

	/**
	 * @brief Destructor por defecto.
	 */
	~HierarchyComponent() = default;

	/**
	 * @brief Inicializa el componente.
	 */
	void
		init() override {}

	/**
	 * @brief Actualiza la lógica del componente.
	 * @param float Tiempo transcurrido (deltaTime).
	 */
	void
		update(float) override {}

	/**
	 * @brief Renderiza visualizaciones de depuración del grafo de escena (si aplica).
	 * @param deviceContext Contexto del dispositivo de renderizado.
	 */
	void
		render(DeviceContext& deviceContext) override {}

	/**
	 * @brief Limpia el nodo, desconectándolo de su padre y vaciando su lista de hijos.
	 */
	void
		destroy() override {
		m_children.clear();
		m_parent = nullptr;
	}

	// API SceneGraph

	/**
	 * @brief Establece la entidad padre para este nodo.
	 * @param parent Puntero a la entidad que actuará como padre.
	 */
	void
		setParent(Entity* parent) {
		m_parent = parent;
	}

	/**
	 * @brief Comprueba si esta entidad es la raíz de su propia jerarquía (no tiene padre).
	 * @return bool True si es un nodo raíz.
	 */
	bool
		isRoot() const {
		return m_parent == nullptr;
	}

	/**
	 * @brief Verifica si la entidad tiene descendencia directa.
	 * @return bool True si la lista de hijos no está vacía.
	 */
	bool
		hasChildren() const {
		return !m_children.empty();
	}

	/**
	 * @brief Vincula una entidad como hija de este nodo, evitando duplicados.
	 * @param child Puntero a la entidad hija a añadir.
	 */
	void
		addChild(Entity* child) {
		if (!child) {
			return;
		}

		if (std::find(m_children.begin(), m_children.end(), child) != m_children.end()) {
			return;
		}
		m_children.push_back(child);
	}

	/**
	 * @brief Desvincula a una entidad de la lista de hijos de este nodo.
	 * @param child Puntero a la entidad hija a remover.
	 */
	void
		removeChild(Entity* child) {
		if (!child) return;

		m_children.erase(
			std::remove(m_children.begin(), m_children.end(), child),
			m_children.end()
		);
	}

public:
	Entity* m_parent = nullptr;              ///< Referencia directa a la entidad padre en la jerarquía.
	std::vector<Entity*> m_children;         ///< Colección de referencias a todas las entidades hijas directas.
};