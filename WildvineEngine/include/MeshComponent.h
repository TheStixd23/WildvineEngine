#pragma once
#include "Prerequisites.h"
#include "ECS\Component.h"
class DeviceContext;
/**
 * @class MeshComponent
 * @brief Componente ECS que almacena la informaci�n de geometr�a (malla) de un actor.
 *
 * Un @c MeshComponent contiene los v�rtices e �ndices que describen la geometr�a de un objeto.
 * Forma parte del sistema ECS y se asocia a entidades como @c Actor.
 *
 * La malla incluye:
 * - Lista de v�rtices (posici�n, normal, UV, etc.).
 * - Lista de �ndices que definen las primitivas (tri�ngulos, l�neas).
 * - Contadores de v�rtices e �ndices.
 */
class
	MeshComponent : public Component {
public:
	/**
	 * @brief Constructor por defecto.
	 *
	 * Inicializa el componente de malla con cero v�rtices e �ndices
	 * y lo registra como tipo @c MESH en el sistema ECS.
	 */
	MeshComponent() : m_numVertex(0), m_numIndex(0), Component(ComponentType::MESH) {}

	/**
	 * @brief Destructor virtual por defecto.
	 */
	virtual
		~MeshComponent() = default;

	/**
	 * @brief Inicializa el componente de malla.
	 *
	 * M�todo heredado de @c Component.
	 * Puede usarse para reservar memoria o cargar datos en mallas derivadas.
	 */
	void
		init() override {};

	/**
	 * @brief Actualiza la malla.
	 *
	 * M�todo heredado de @c Component.
	 * �til para actualizar animaciones de v�rtices, morphing u otros procesos relacionados.
	 *
	 * @param deltaTime Tiempo transcurrido desde la �ltima actualizaci�n.
	 */
	void
		update(float deltaTime) override {};

	/**
	 * @brief Renderiza la malla.
	 *
	 * M�todo heredado de @c Component.
	 * Normalmente se usar�a junto con @c DeviceContext para dibujar buffers
	 * asociados a la malla.
	 *
	 * @param deviceContext Contexto del dispositivo para operaciones gr�ficas.
	 */
	void
		render(DeviceContext& deviceContext) override {};

	/**
	 * @brief Libera los recursos asociados al componente de malla.
	 *
	 * M�todo heredado de @c Component.
	 * En implementaciones m�s complejas, puede liberar buffers de GPU.
	 */
	void
		destroy() override {};

public:
	/**
	 * @brief Nombre de la malla.
	 */
	std::string m_name;

	/**
	 * @brief Lista de v�rtices de la malla.
	 */
	std::vector<SimpleVertex> m_vertex;
	std::vector<SkyboxVertex> m_skyVertex;

	/**
	 * @brief Lista de �ndices que definen las primitivas de la malla.
	 */
	std::vector<unsigned int> m_index;

	/**
	 * @brief N�mero total de v�rtices en la malla.
	 */
	int m_numVertex;

	/**
	 * @brief N�mero total de �ndices en la malla.
	 */
	int m_numIndex;


	/**
	 * @brief Slot del material original del archivo importado.
	 *
	 * Model3D asigna este valor al separar una geometria FBX/OBJ por
	 * materiales. El renderer lo usa para seleccionar la instancia de
	 * material correcta en cada submalla.
	 */
	unsigned int m_materialSlot = 0;
};
