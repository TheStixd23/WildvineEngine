/**
 * @file ForwardRenderer.h
 * @brief Declara la API de ForwardRenderer dentro del subsistema Rendering.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Buffer.h"
#include "DepthStencilState.h"
#include "DepthStencilView.h"
#include "RasterizerState.h"
#include "Rendering/RenderScene.h"
#include "Rendering/RenderTypes.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "EngineUtilities/Utilities/EditorViewportPass.h"

class Device;
class DeviceContext;
class Camera;
class Material;

/**
 * @class ForwardRenderer
 * @brief Ejecuta el pipeline de render forward del motor.
 *
 * Esta clase construye colas opacas y transparentes, genera recursos de sombras,
 * actualiza buffers por frame y compone el resultado final dentro del viewport del editor.
 */
class
	ForwardRenderer {
public:
	/**
	 * @brief Inicializa buffers, shaders y estados del renderer.
	 * @param device Referencia al dispositivo gráfico utilizado para crear los recursos.
	 * @return HRESULT Código de éxito o error tras la inicialización.
	 */
	HRESULT init(Device& device);

	/**
	 * @brief Reconstuye los recursos dependientes del tamano del viewport.
	 * @param device Referencia al dispositivo gráfico.
	 * @param width Nuevo ancho en píxeles.
	 * @param height Nuevo alto en píxeles.
	 */
	void resize(Device& device, unsigned int width, unsigned int height);

	/**
	 * @brief Actualiza constantes globales usadas por el frame actual.
	 * @param camera Cámara desde la cual se está observando la escena.
	 * @param scene Escena actual que contiene las luces y objetos.
	 * @param deviceContext Contexto del dispositivo para la actualización de buffers.
	 */
	void updatePerFrame(const Camera& camera, const RenderScene& scene, DeviceContext& deviceContext);

	/**
	 * @brief Renderiza la escena completa sobre el `EditorViewportPass`.
	 * @param deviceContext Contexto del dispositivo gráfico para emitir los comandos de dibujado.
	 * @param camera Cámara activa.
	 * @param scene Escena a renderizar.
	 * @param viewportPass Pase de destino para el resultado del renderizado.
	 */
	void render(DeviceContext& deviceContext,
		const Camera& camera,
		RenderScene& scene,
		EditorViewportPass& viewportPass);

	/**
	 * @brief Libera los recursos internos del renderer.
	 */
	void destroy();

	/**
	 * @brief Obtiene la vista de recurso del mapa de sombras principal.
	 * @return ID3D11ShaderResourceView* Puntero al SRV del Shadow Map.
	 */
	ID3D11ShaderResourceView* getShadowMapSRV() const { return m_shadowDepthSRV.m_textureFromImg; }

	/**
	 * @brief Obtiene la vista de recurso del pase de depuración de sombras.
	 * @return ID3D11ShaderResourceView* Puntero al SRV del pase de depuración.
	 */
	ID3D11ShaderResourceView* getPreShadowSRV() const { return m_preShadowDebugPass.getSRV(); }

private:
	/** @brief Clasifica los objetos de la escena en colas opacas y transparentes. */
	void buildQueues(RenderScene& scene, const Camera& camera);
	/** @brief Ejecuta un pase preliminar para depurar el estado de las sombras. */
	void renderPreShadowDebugPass(DeviceContext& deviceContext, RenderScene& scene);
	/** @brief Genera el mapa de profundidad de las sombras. */
	void renderShadowPass(DeviceContext& deviceContext);
	/** @brief Dibuja la cola de objetos completamente opacos. */
	void renderOpaquePass(DeviceContext& deviceContext);
	/** @brief Dibuja la cola de objetos transparentes o translúcidos. */
	void renderTransparentPass(DeviceContext& deviceContext);
	/** @brief Dibuja el entorno del cielo (Skybox). */
	void renderSkyboxPass(DeviceContext& deviceContext, RenderScene& scene);
	/** @brief Dibuja un objeto individual especificando su tipo de pase. */
	void renderObject(DeviceContext& deviceContext, const RenderObject& object, RenderPassType passType);
	/** @brief Dibuja un objeto individual específicamente para la generación del mapa de sombras. */
	void renderShadowObject(DeviceContext& deviceContext, const RenderObject& object);

	/** @brief Crea las texturas y vistas necesarias para el Shadow Mapping. */
	HRESULT createShadowResources(Device& device);
	/** @brief Recalcula las matrices de proyección y vista para las luces de la escena. */
	void updateLightMatrices(const Camera& camera, const RenderScene& scene);
	/** @brief Inicializa los estados de mezcla (blend states) soportados. */
	HRESULT createBlendStates(Device& device);
	/** @brief Determina el estado de mezcla adecuado según el material proporcionado. */
	ID3D11BlendState* resolveBlendState(const Material* material) const;

private:
	Buffer m_perFrameBuffer;             /**< Buffer constante con datos globales del frame actual. */
	Buffer m_perObjectBuffer;            /**< Buffer constante con transformaciones del objeto actual. */
	Buffer m_perMaterialBuffer;          /**< Buffer constante con propiedades del material actual. */

	DepthStencilState m_transparentDepthStencil; /**< Estado de profundidad modificado para objetos transparentes. */

	ID3D11BlendState* m_alphaBlendState = nullptr;         /**< Estado para mezcla alfa estándar (Alpha Blending). */
	ID3D11BlendState* m_opaqueBlendState = nullptr;        /**< Estado por defecto para objetos sólidos (sin mezcla). */
	ID3D11BlendState* m_additiveBlendState = nullptr;      /**< Estado para mezcla aditiva (ej. partículas brillantes). */
	ID3D11BlendState* m_premultipliedBlendState = nullptr; /**< Estado para alfa premultiplicado. */
	float m_blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };   /**< Factores globales de mezcla. */

	Texture m_shadowDepthTexture;        /**< Textura que almacena la profundidad desde la luz. */
	Texture m_shadowDepthSRV;            /**< SRV para leer el mapa de sombras en los shaders. */
	DepthStencilView m_shadowDSV;        /**< DSV para escribir en el mapa de sombras. */
	ShaderProgram m_shadowShader;        /**< Shader dedicado a la generación de sombras. */
	RasterizerState m_shadowRasterizer;  /**< Estado de rasterizador específico para evitar acné de sombras. */
	unsigned int m_shadowMapSize = 2048; /**< Resolución cuadrada del mapa de sombras. */

	EditorViewportPass m_preShadowDebugPass; /**< Pase auxiliar para previsualizar el mapa de sombras en el editor. */
	bool m_applyShadows = true;              /**< Bandera que indica si se calcularán sombras en el frame actual. */

	CBPerFrame m_cbPerFrame{};           /**< Caché local de los datos por frame. */
	CBPerObject m_cbPerObject{};         /**< Caché local de los datos por objeto. */
	CBPerMaterial m_cbPerMaterial{};     /**< Caché local de los datos del material. */

	std::vector<const RenderObject*> m_opaqueQueue;      /**< Lista de objetos procesados en el pase opaco. */
	std::vector<const RenderObject*> m_transparentQueue; /**< Lista de objetos procesados en el pase transparente. */
};