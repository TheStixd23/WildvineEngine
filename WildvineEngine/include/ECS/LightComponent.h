/**
 * @file LightComponent.h
 * @brief Declara la API de LightComponent dentro del subsistema ECS de WildvineEngine.
 * @ingroup ecs
 */
#pragma once
#include "Prerequisites.h"
#include "ECS/Component.h"
#include "Rendering/RenderTypes.h"

class DeviceContext;

/**
 * @class LightComponent
 * @brief Componente que otorga propiedades de iluminación a una entidad en el ECS.
 *
 * Define los atributos físicos de una luz (color, intensidad, rango, tipo) mediante la
 * estructura `LightData` y establece si debe interactuar con el sistema de sombras del motor.
 */
class
	LightComponent : public Component {
public:
	/**
	 * @brief Constructor por defecto del componente de luz.
	 */
	LightComponent()
		: Component(ComponentType::NONE) {
	}

	/**
	 * @brief Inicializa el estado base de la componente en la entidad.
	 */
	void init() override {}

	/**
	 * @brief Actualiza la lógica de la luz frame a frame (útil para luces dinámicas o animadas).
	 * @param deltaTime Tiempo transcurrido en segundos desde el último frame.
	 */
	void update(float deltaTime) override {}

	/**
	 * @brief Dibuja representaciones visuales de la componente, generalmente para debug.
	 * @param deviceContext Contexto del dispositivo gráfico utilizado para emitir comandos.
	 */
	void render(DeviceContext& deviceContext) override {}

	/**
	 * @brief Libera los recursos que la componente haya podido reservar.
	 */
	void destroy() override {}

	/**
	 * @brief Obtiene una referencia modificable a la estructura de datos físicos de la luz.
	 * @return LightData& Referencia a los datos (color, intensidad, tipo, etc.).
	 */
	LightData& getLightData() { return m_light; }

	/**
	 * @brief Obtiene una referencia de solo lectura a la estructura de datos físicos de la luz.
	 * @return const LightData& Referencia constante a los datos.
	 */
	const LightData& getLightData() const { return m_light; }

	/**
	 * @brief Configura si esta luz debe generar pasadas en el Shadow Map.
	 * @param value True para proyectar sombras, false para ignorarlas.
	 */
	void setCastShadow(bool value) { m_castShadow = value; }

	/**
	 * @brief Indica si la luz está configurada para emitir sombras.
	 * @return bool True si la generación de sombras está habilitada.
	 */
	bool canCastShadow() const { return m_castShadow; }

private:
	LightData m_light;          ///< Estructura que agrupa las características físicas (tipo, color, rango, dirección).
	bool m_castShadow = false;  ///< Determina si el renderer debe calcular oclusión para esta luz.
};