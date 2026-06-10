/**
 * @file Skybox.h
 * @brief Declara la API de Skybox dentro del subsistema Utilities de WildvineEngine.
 * @ingroup utilities
 */
#pragma once
#include "Prerequisites.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Buffer.h"
#include "SamplerState.h"
#include "Model3D.h"
#include "RasterizerState.h"
#include "DepthStencilState.h"
#include "EngineUtilities\Utilities\Camera.h"
#include "ECS\Actor.h"

class Device;
class DeviceContext;

/**
 * @class Skybox
 * @brief Administra y renderiza el entorno cúbico de fondo (Skybox) de la escena.
 *
 * Encapsula la lógica necesaria para dibujar un cubo infinito alrededor de la cámara
 * utilizando una textura tipo Cubemap. Maneja sus propios estados de profundidad
 * y rasterizado para garantizar que siempre se dibuje por detrás de toda la geometría.
 */
class
	Skybox {
public:
	/**
	 * @brief Constructor por defecto.
	 */
	Skybox() = default;

	/**
	 * @brief Destructor por defecto.
	 */
	~Skybox() = default;

	/**
	 * @brief Inicializa los shaders, el modelo cúbico y los estados gráficos del Skybox.
	 * @param device Referencia al dispositivo gráfico para crear recursos.
	 * @param deviceContext Puntero al contexto del dispositivo.
	 * @param cubemap Textura configurada como Cubemap que servirá como entorno visual.
	 * @return HRESULT S_OK si se instancian correctamente todos los recursos.
	 */
	HRESULT
		init(Device& device, DeviceContext* deviceContext, Texture& cubemap);

	/**
	 * @brief Actualiza la posición y los buffers constantes del Skybox relativos a la cámara.
	 * @param deviceContext Contexto del dispositivo utilizado para mapear los buffers.
	 * @param camera Cámara principal desde la cual se observa la escena.
	 */
	void
		update(DeviceContext& deviceContext, Camera& camera);

	/**
	 * @brief Ejecuta el dibujado del entorno sobre el render target activo.
	 * @param deviceContext Contexto del dispositivo gráfico para emitir los comandos.
	 */
	void
		render(DeviceContext& deviceContext);

	/**
	 * @brief Libera los recursos de memoria y GPU ocupados por el Skybox.
	 */
	void
		destroy() {}

private:
	ShaderProgram m_shaderProgram;         ///< Programa de shaders dedicado a proyectar el entorno.
	Buffer m_constantBuffer;               ///< Buffer constante (GPU) con las matrices de vista y proyección.
	SamplerState m_samplerState;           ///< Estado de muestreador para interpolar el cubemap.
	RasterizerState m_rasterizerState;     ///< Estado que desactiva el culling (o invierte caras) para ver desde adentro.
	DepthStencilState m_depthStencilState; ///< Estado que fuerza prueba de profundidad con "LessEqual" y desactiva la escritura Z.
	Texture m_skyboxTexture;               ///< La textura cúbica (Cubemap) del entorno.
	Model3D* m_cubeModel = nullptr;        ///< Puntero al modelo geométrico en forma de cubo.
	EU::TSharedPointer<Actor> m_skybox;    ///< Actor base para acoplar el Skybox en la jerarquía del ECS o del motor espacial.

};