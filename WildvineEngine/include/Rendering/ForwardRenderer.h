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
class ForwardRenderer {
public:
	/**
	 * @brief Inicializa buffers, shaders y estados del renderer.
	 * @param device Referencia al dispositivo gráfico utilizado para la creación de recursos.
	 * @return HRESULT Código de error indicando el éxito (S_OK) o fracaso de la inicialización.
	 */
	HRESULT init(Device& device);

	/**
	 * @brief Reconstruye los recursos dependientes del tamaño del viewport.
	 * @param device Referencia al dispositivo gráfico.
	 * @param width Nuevo ancho del viewport en píxeles.
	 * @param height Nuevo alto del viewport en píxeles.
	 */
	void resize(Device& device, unsigned int width, unsigned int height);

	/**
	 * @brief Actualiza las constantes globales (Constant Buffers) usadas por el frame actual.
	 * @param camera Cámara activa desde la cual se observa la escena.
	 * @param scene Escena actual que contiene datos de iluminación y entorno.
	 * @param deviceContext Contexto del dispositivo para actualizar la memoria de la GPU.
	 */
	void updatePerFrame(const Camera& camera, const RenderScene& scene, DeviceContext& deviceContext);

	/**
	 * @brief Renderiza la escena completa sobre el `EditorViewportPass`.
	 * @param deviceContext Contexto del dispositivo utilizado para emitir comandos de dibujo.
	 * @param camera Cámara activa para el renderizado.
	 * @param scene Escena que contiene los objetos a dibujar.
	 * @param viewportPass Pase de renderizado del editor donde se volcará el resultado final.
	 */
	void render(DeviceContext& deviceContext,
		const Camera& camera,
		RenderScene& scene,
		EditorViewportPass& viewportPass);

	/**
	 * @brief Libera de forma segura todos los recursos internos del renderer en la GPU.
	 */
	void destroy();

	/**
	 * @brief Obtiene la vista de recurso del shader (SRV) del mapa de sombras.
	 * @return ID3D11ShaderResourceView* Puntero al SRV del mapa de profundidad de sombras.
	 */
	ID3D11ShaderResourceView* getShadowMapSRV() const { return m_shadowDepthSRV.m_textureFromImg; }

	/**
	 * @brief Obtiene la vista de recurso del shader (SRV) del pase previo de sombras (Debug).
	 * @return ID3D11ShaderResourceView* Puntero al SRV del render pass de debug.
	 */
	ID3D11ShaderResourceView* getPreShadowSRV() const { return m_preShadowDebugPass.getSRV(); }

private:
	/**
	 * @brief Construye las colas de renderizado separando la geometría opaca de la transparente.
	 * @param scene Escena que contiene los objetos a clasificar.
	 * @param camera Cámara utilizada para posibles comprobaciones de visibilidad (Culling).
	 */
	void buildQueues(RenderScene& scene, const Camera& camera);

	/**
	 * @brief Ejecuta un pase de renderizado de depuración previo al cálculo de sombras.
	 * @param deviceContext Contexto del dispositivo gráfico.
	 * @param scene Escena a renderizar.
	 */
	void renderPreShadowDebugPass(DeviceContext& deviceContext, RenderScene& scene);

	/**
	 * @brief Ejecuta el pase de renderizado para generar el mapa de sombras desde la perspectiva de la luz.
	 * @param deviceContext Contexto del dispositivo gráfico.
	 */
	void renderShadowPass(DeviceContext& deviceContext);

	/**
	 * @brief Renderiza secuencialmente la cola de geometría opaca.
	 * @param deviceContext Contexto del dispositivo gráfico.
	 */
	void renderOpaquePass(DeviceContext& deviceContext);

	/**
	 * @brief Renderiza la cola de geometría transparente.
	 * @param deviceContext Contexto del dispositivo gráfico.
	 */
	void renderTransparentPass(DeviceContext& deviceContext);

	/**
	 * @brief Renderiza el skybox o entorno de fondo de la escena si está habilitado.
	 * @param deviceContext Contexto del dispositivo gráfico.
	 * @param scene Escena que contiene la información del skybox.
	 */
	void renderSkyboxPass(DeviceContext& deviceContext, RenderScene& scene);

	/**
	 * @brief Despacha los comandos de dibujo para un objeto individual.
	 * @param deviceContext Contexto del dispositivo gráfico.
	 * @param object Objeto que contiene malla y material a dibujar.
	 * @param passType Identificador del pase de renderizado actual para escoger técnicas apropiadas.
	 */
	void renderObject(DeviceContext& deviceContext, const RenderObject& object, RenderPassType passType);

	/**
	 * @brief Despacha los comandos de dibujo para un objeto exclusivamente sobre el depth buffer de sombras.
	 * @param deviceContext Contexto del dispositivo gráfico.
	 * @param object Objeto a dibujar.
	 */
	void renderShadowObject(DeviceContext& deviceContext, const RenderObject& object);

	/**
	 * @brief Crea las texturas, vistas (DSV/SRV) y estados necesarios para el algoritmo de sombras.
	 * @param device Dispositivo gráfico.
	 * @return HRESULT Código de error de la operación.
	 */
	HRESULT createShadowResources(Device& device);

	/**
	 * @brief Calcula y actualiza las matrices necesarias (Vista, Proyección) para la luz principal.
	 * @param camera Cámara de la vista actual (útil para sombras dinámicas en cascada o centradas).
	 * @param scene Escena que provee los datos de iluminación direccional.
	 */
	void updateLightMatrices(const Camera& camera, const RenderScene& scene);

	/**
	 * @brief Crea los diversos estados de mezcla (Blend States) soportados por el motor.
	 * @param device Dispositivo gráfico.
	 * @return HRESULT Código de error de la operación.
	 */
	HRESULT createBlendStates(Device& device);

	/**
	 * @brief Determina y devuelve el estado de mezcla D3D11 a aplicar basándose en las propiedades de un material.
	 * @param material Puntero al material a evaluar.
	 * @return ID3D11BlendState* Puntero al estado de mezcla correspondiente.
	 */
	ID3D11BlendState* resolveBlendState(const Material* material) const;

private:
	Buffer m_perFrameBuffer;             ///< Constant buffer para información fija durante todo el frame.
	Buffer m_perObjectBuffer;            ///< Constant buffer actualizado por cada objeto (matrices locales/mundo).
	Buffer m_perMaterialBuffer;          ///< Constant buffer para las propiedades visuales de un material.

	DepthStencilState m_transparentDepthStencil; ///< Estado Depth/Stencil particular para mallas transparentes (ej. depth test activado, depth write desactivado).

	ID3D11BlendState* m_alphaBlendState = nullptr;         ///< Estado para mezcla por transparencia Alpha (SrcAlpha, InvSrcAlpha).
	ID3D11BlendState* m_opaqueBlendState = nullptr;        ///< Estado por defecto para geometría completamente opaca (sin blending).
	ID3D11BlendState* m_additiveBlendState = nullptr;      ///< Estado para mezcla aditiva (partículas, brillos).
	ID3D11BlendState* m_premultipliedBlendState = nullptr; ///< Estado optimizado para texturas con Alpha Premultiplicado.
	float m_blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };   ///< Factores globales pasados al pipeline de mezcla.

	Texture m_shadowDepthTexture;        ///< Textura de hardware que almacena la profundidad desde el punto de vista de la luz.
	Texture m_shadowDepthSRV;            ///< Recurso que permite muestrear el depth buffer de sombras en los fragment shaders.
	DepthStencilView m_shadowDSV;        ///< Recurso de vista usado como target para escribir en `m_shadowDepthTexture`.
	ShaderProgram m_shadowShader;        ///< Shaders simplificados para el renderizado exclusivo de profundidad (sin pixel shader complejo).
	RasterizerState m_shadowRasterizer;  ///< Estado del rasterizador para sombras (incluye slope-scaled depth bias).
	unsigned int m_shadowMapSize = 2048; ///< Resolución cuadrada (Width x Height) de la textura de sombras.

	EditorViewportPass m_preShadowDebugPass; ///< Contenedor FBO/Target usado para el debug paso a paso en el editor de visualización.
	bool m_applyShadows = true;              ///< Switch interno para activar o desactivar la proyección y dibujado de sombras.

	CBPerFrame m_cbPerFrame{};           ///< Memoria CPU espejo para mapear en el buffer constante de Frame.
	CBPerObject m_cbPerObject{};         ///< Memoria CPU espejo para mapear en el buffer constante de Objeto.
	CBPerMaterial m_cbPerMaterial{};     ///< Memoria CPU espejo para mapear en el buffer constante de Material.

	std::vector<const RenderObject*> m_opaqueQueue;      ///< Lista dinámica pre-calculada de objetos que se dibujarán como opacos en el frame actual.
	std::vector<const RenderObject*> m_transparentQueue; ///< Lista dinámica pre-calculada de objetos transparentes. Generalmente ordenada de atrás hacia adelante.
};