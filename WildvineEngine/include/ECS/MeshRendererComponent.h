/**
 * @file MeshRendererComponent.h
 * @brief Declara la API de MeshRendererComponent dentro del subsistema ECS de WildvineEngine.
 * @ingroup ecs
 */
#pragma once
#include "Prerequisites.h"
#include "ECS/Component.h"

class Mesh;
class MaterialInstance;
class DeviceContext;

/**
 * @class MeshRendererComponent
 * @brief Componente que vincula la geometría (Mesh) con sus materiales para renderizar una entidad.
 *
 * Otorga a una entidad dentro del ECS la capacidad de ser dibujada en pantalla,
 * permitiendo asignar tanto un único material base como una lista de múltiples
 * instancias de material (útil para modelos complejos con varias submallas).
 */
class
	MeshRendererComponent : public Component {
public:
	/**
	 * @brief Constructor por defecto del componente de renderizado de malla.
	 */
	MeshRendererComponent()
		: Component(ComponentType::MESH) {
	}

	/**
	 * @brief Inicializa los recursos internos de la componente.
	 */
	void init() override {}

	/**
	 * @brief Actualiza la lógica de la componente frame a frame.
	 * @param deltaTime Tiempo en segundos desde el último frame.
	 */
	void update(float deltaTime) override {}

	/**
	 * @brief Dibuja representaciones adicionales o depuración para la malla.
	 * @param deviceContext Contexto del dispositivo gráfico utilizado para emitir comandos.
	 */
	void render(DeviceContext& deviceContext) override {}

	/**
	 * @brief Libera referencias o recursos atados a la componente.
	 */
	void destroy() override {}

	/**
	 * @brief Asigna la malla geométrica que utilizará este componente.
	 * @param mesh Puntero al objeto Mesh cargado.
	 */
	void setMesh(Mesh* mesh) { m_mesh = mesh; }

	/**
	 * @brief Obtiene la malla geométrica asociada a la entidad.
	 * @return Mesh* Puntero a la malla.
	 */
	Mesh* getMesh() const { return m_mesh; }

	/**
	 * @brief Asigna un único material a la malla, limpiando la lista de materiales previos.
	 * @param materialInstance Puntero a la instancia de material principal.
	 */
	void setMaterialInstance(MaterialInstance* materialInstance) {
		m_materialInstance = materialInstance;
		m_materialInstances.clear();
		if (materialInstance) {
			m_materialInstances.push_back(materialInstance);
		}
	}

	/**
	 * @brief Obtiene la instancia de material principal (el primer slot).
	 * @return MaterialInstance* Puntero al material principal.
	 */
	MaterialInstance* getMaterialInstance() const { return m_materialInstance; }

	/**
	 * @brief Asigna una lista completa de materiales para cada submalla del modelo.
	 * @param materialInstances Vector que contiene las instancias de materiales.
	 */
	void setMaterialInstances(const std::vector<MaterialInstance*>& materialInstances) {
		m_materialInstances = materialInstances;
		m_materialInstance = m_materialInstances.empty() ? nullptr : m_materialInstances.front();
	}

	/**
	 * @brief Añade un nuevo material a la lista de materiales de la entidad.
	 * @param materialInstance Puntero al material a añadir.
	 */
	void addMaterialInstance(MaterialInstance* materialInstance) {
		if (!materialInstance) {
			return;
		}
		if (!m_materialInstance) {
			m_materialInstance = materialInstance;
		}
		m_materialInstances.push_back(materialInstance);
	}

	/**
	 * @brief Obtiene la lista completa de instancias de materiales asignadas.
	 * @return const std::vector<MaterialInstance*>& Referencia al arreglo de materiales.
	 */
	const std::vector<MaterialInstance*>& getMaterialInstances() const { return m_materialInstances; }

	/**
	 * @brief Verifica si el objeto debe ser dibujado por el renderer.
	 * @return bool True si es visible.
	 */
	bool isVisible() const { return m_visible; }

	/**
	 * @brief Controla la visibilidad global de la malla geométrica.
	 * @param visible True para habilitar su dibujado, false para ocultarlo.
	 */
	void setVisible(bool visible) { m_visible = visible; }

	/**
	 * @brief Indica si el objeto proyectará sombras en la escena.
	 * @return bool True si participa en el Shadow Map.
	 */
	bool canCastShadow() const { return m_castShadow; }

	/**
	 * @brief Define si el objeto debe proyectar sombras físicas sobre el entorno.
	 * @param value True para emitir sombras, false en caso contrario.
	 */
	void setCastShadow(bool value) { m_castShadow = value; }

private:
	Mesh* m_mesh = nullptr;                               ///< Puntero a la topología y vértices del modelo.
	MaterialInstance* m_materialInstance = nullptr;       ///< Referencia rápida al material principal (slot 0).
	std::vector<MaterialInstance*> m_materialInstances;   ///< Lista de materiales en caso de tener múltiples submallas.
	bool m_visible = true;                                ///< Bandera de visibilidad global.
	bool m_castShadow = true;                             ///< Bandera que indica si participa en los pases de sombras.
};