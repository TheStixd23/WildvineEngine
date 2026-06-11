/**
 * @file DeferredRenderer.h
 * @brief Declara la API de DeferredRenderer dentro del subsistema Rendering.
 * @ingroup rendering
 */
#pragma once
#include "Buffer.h"
#include "DepthStencilState.h"
#include "DepthStencilView.h"
#include "RasterizerState.h"
#include "Rendering/ISceneRenderer.h"
#include "Rendering/RenderScene.h"
#include "Rendering/RenderTypes.h"
#include "SamplerState.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "EngineUtilities/Utilities/EditorViewportPass.h"

class Device;
class DeviceContext;
class Camera;
class Material;

/**
 * @class DeferredRenderer
 * @brief Implementa un pipeline diferido con GBuffer y lighting pass.
 *
 * El renderer usa deferred shading para superficies opacas y mantiene un subpass
 * forward para transparencias, de modo que el pipeline del editor siga funcionando
 * con el contenido actual del engine.
 */
class
	DeferredRenderer : public ISceneRenderer {
public:
	/**
	 * @brief Inicializa el renderizador y sus recursos en la GPU.
	 * @param device Referencia al dispositivo logico (Device).
	 * @return HRESULT S_OK si se inicializa correctamente.
	 */
	HRESULT
		init(Device& device) override;

	/**
	 * @brief Reajusta los targets y resoluciones del renderizador al cambiar el tamano de la ventana.
	 * @param device Referencia al dispositivo.
	 * @param width Nuevo ancho en pixeles.
	 * @param height Nuevo alto en pixeles.
	 */
	void
		resize(Device& device, unsigned int width, unsigned int height) override;

	/**
	 * @brief Ejecuta el dibujado general de la escena utilizando Deferred Shading.
	 * @param deviceContext Contexto del dispositivo para emitir comandos.
	 * @param camera Camara actual de renderizado.
	 * @param scene Contenedor de la geometria y luces.
	 * @param viewportPass El paso del viewport donde sera dibujado.
	 */
	void
		render(DeviceContext& deviceContext,
			const Camera& camera,
			RenderScene& scene,
			EditorViewportPass& viewportPass) override;

	/**
	 * @brief Libera de memoria los buffers, shaders y targets creados.
	 */
	void
		destroy() override;

	/**
	 * @brief Obtiene la vista de recurso correspondiente al Mapa de Sombras.
	 * @return ID3D11ShaderResourceView*
	 */
	ID3D11ShaderResourceView*
		getShadowMapSRV() const override { return m_shadowDepthSRV.m_textureFromImg; }

	/**
	 * @brief Obtiene la vista de recurso de depuracion para el pre-pase de sombras.
	 * @return ID3D11ShaderResourceView*
	 */
	ID3D11ShaderResourceView*
		getPreShadowSRV() const override { return m_preShadowDebugPass.getSRV(); }

	/**
	 * @brief Obtiene la vista de recurso del pase del G-Buffer de Albedo y Metallic.
	 * @return ID3D11ShaderResourceView*
	 */
	ID3D11ShaderResourceView*
		getGBufferAlbedoMetallicSRV() const override { return m_gBufferAlbedoMetallicSRV.m_textureFromImg; }

	/**
	 * @brief Obtiene la vista de recurso del pase del G-Buffer de Normales y Roughness.
	 * @return ID3D11ShaderResourceView*
	 */
	ID3D11ShaderResourceView*
		getGBufferNormalRoughnessSRV() const override { return m_gBufferNormalRoughnessSRV.m_textureFromImg; }

	/**
	 * @brief Obtiene la vista de recurso del pase del G-Buffer de World Position y Ambient Occlusion.
	 * @return ID3D11ShaderResourceView*
	 */
	ID3D11ShaderResourceView*
		getGBufferWorldAoSRV() const override { return m_gBufferWorldAoSRV.m_textureFromImg; }

	/**
	 * @brief Obtiene la vista de recurso del pase del G-Buffer de canal Emisivo y Alpha.
	 * @return ID3D11ShaderResourceView*
	 */
	ID3D11ShaderResourceView*
		getGBufferEmissiveAlphaSRV() const override { return m_gBufferEmissiveAlphaSRV.m_textureFromImg; }

	/**
	 * @brief Conmuta la visibilidad de la capa de depuracion visual para las sombras.
	 * @param enabled Booleano de activacion.
	 */
	void
		setShadowFactorDebugEnabled(bool enabled) override { m_shadowFactorDebugEnabled = enabled; }

	/**
	 * @brief Cambia el modo de depuracion del G-Buffer (vista de Albedo, Normales, etc).
	 * @param mode El identificador en formato int.
	 */
	void
		setDeferredDebugViewMode(int mode) override { m_deferredDebugViewMode = mode; }

	/**
	 * @brief Obtiene un string con el nombre descriptivo de este renderer.
	 * @return const char*
	 */
	const char*
		getDebugName() const override { return "DeferredRenderer"; }

private:
	/** @brief Clasifica los objetos visibles en colas opacas y transparentes. */
	void buildQueues(RenderScene& scene, const Camera& camera);
	/** @brief Actualiza los buffers constantes a nivel fotograma. */
	void updatePerFrame(const Camera& camera, const RenderScene& scene, DeviceContext& deviceContext);
	/** @brief Actualiza la proyeccion ortografica y matriz de la luz direccional principal. */
	void updateLightMatrices(const Camera& camera, const RenderScene& scene);
	/** @brief Orquesta el pipeline general dibujando los pases sobre el target objetivo. */
	void renderSceneToTarget(DeviceContext& deviceContext, RenderScene& scene, EditorViewportPass& targetPass, bool applyShadows);
	/** @brief Vincula los multiples Render Targets del G-Buffer. */
	void bindGBufferTargets(DeviceContext& deviceContext, ID3D11DepthStencilView* depthStencilView);
	/** @brief Restaurar el Render Target View por defecto o destino final. */
	void bindFinalTarget(DeviceContext& deviceContext, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthStencilView);
	/** @brief Limpia el estado de los SRV para evitar colisiones de recurso con DX11. */
	void clearDeferredSRVs(DeviceContext& deviceContext);
	/** @brief Ejecuta el trazado de la geometria estandar contra el G-Buffer. */
	void renderGeometryPass(DeviceContext& deviceContext);
	/** @brief Emite un objeto de la cola opaca hacia los shaders pasivos. */
	void renderGeometryObject(DeviceContext& deviceContext, const RenderObject& object);
	/** @brief Procesa el G-Buffer a traves del shader direccional/diferido. */
	void renderLightingPass(DeviceContext& deviceContext);
	/** @brief Renderiza el Skybox usando manipulacion de Depth Stencil. */
	void renderSkyboxPass(DeviceContext& deviceContext, RenderScene& scene);
	/** @brief Pinta la geometria indexada de la cola transparente de forma Forward. */
	void renderTransparentPass(DeviceContext& deviceContext);
	/** @brief Subrutina para despachar geometria en un pase tradicional Forward. */
	void renderForwardObject(DeviceContext& deviceContext, const RenderObject& object, RenderPassType passType);
	/** @brief Ejecuta la captura de profundidad de los modelos desde la vista de la luz. */
	void renderShadowPass(DeviceContext& deviceContext);
	/** @brief Emite el subset opaco hacia el buffer de profundidad para generar sombras. */
	void renderShadowObject(DeviceContext& deviceContext, const RenderObject& object);
	/** @brief Pre-aloja memoria en VRAM para las texturas del Shadow Map. */
	HRESULT createShadowResources(Device& device);
	/** @brief Inicializa el grupo completo de texturas para MRT del G-Buffer. */
	HRESULT createGBufferResources(Device& device, unsigned int width, unsigned int height);
	/** @brief Funcion generica de soporte para inicializar targets del G-Buffer. */
	HRESULT createGBufferTarget(Device& device,
		unsigned int width,
		unsigned int height,
		DXGI_FORMAT format,
		Texture& texture,
		Texture& srv,
		RenderTargetView& rtv);
	/** @brief Compila y sube a memoria los Shaders encargados de la iluminacion de pixeles. */
	HRESULT createLightingResources(Device& device);
	/** @brief Genera una primitiva simple (Quad) que llenara la vista para el lighting pass. */
	HRESULT createFullScreenQuad(Device& device);
	/** @brief Crea los estados de operaciones de mezcla Alpha de DX11. */
	HRESULT createBlendStates(Device& device);
	/** @brief Obtiene el BlendState requerido en base a los datos fisicos del Material actual. */
	ID3D11BlendState* resolveBlendState(const Material* material) const;

private:
	Buffer m_perFrameBuffer;             /**< Buffer constante para matrices globales y tiempo. */
	Buffer m_perObjectBuffer;            /**< Buffer constante atado a la posicion de la matriz del modelo. */
	Buffer m_perMaterialBuffer;          /**< Buffer constante con los valores PBR directos. */
	Buffer m_lightingDebugBuffer;        /**< Buffer enlazado para inyectar flags de visualizacion. */
	Buffer m_fullscreenVertexBuffer;     /**< Buffer de vertices para el calculo de iluminacion a pantalla. */
	Buffer m_fullscreenIndexBuffer;      /**< Buffer de indizado del Quad. */

	DepthStencilState m_transparentDepthStencil; /**< Estado de profundidad que evita sobreescritura Z. */
	DepthStencilState m_disabledDepthStencil;    /**< Estado de profundidad en desactivacion forzosa. */
	DepthStencilState m_shadowDepthStencil;      /**< Configuracion para trazado correcto del Shadow Bias. */

	ID3D11BlendState* m_alphaBlendState = nullptr;         /**< Estado para fusion de texturas semitransparentes. */
	ID3D11BlendState* m_opaqueBlendState = nullptr;        /**< Estado opaco sin evaluacion de translucidez. */
	ID3D11BlendState* m_additiveBlendState = nullptr;      /**< Mezclado de suma logica en hardware para luces. */
	ID3D11BlendState* m_premultipliedBlendState = nullptr; /**< Mezclado con factores RGB previamente multiplicados. */
	float m_blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };   /**< Vector 4 multiplicativo global para los estados. */

	Texture m_shadowDepthTexture;        /**< Target Texture con el mapa de profudidad lumina. */
	Texture m_shadowDepthSRV;            /**< Vista de lectura inyectable en la generacion diferida. */
	DepthStencilView m_shadowDSV;        /**< Vista DSV para procesar el Shadow Mapping. */
	ShaderProgram m_shadowShader;        /**< Shader encargado de evaluar la profundidad Z. */
	RasterizerState m_shadowRasterizer;  /**< Rasterizador solido frontal. */
	unsigned int m_shadowMapSize = 2048; /**< Resolucion interna forzada de la cuadricula de sombras. */

	ShaderProgram m_gBufferShader;           /**< Shaders vinculados a la creacion del G-Buffer Base. */
	ShaderProgram m_deferredLightingShader;  /**< Shaders PBR que leen el G-Buffer en Screen-Space. */
	SamplerState m_lightingSampler;          /**< Parametro muestral aplicado a los textmap diferidos. */
	RasterizerState m_fullscreenRasterizer;  /**< Rasterizador generico util al Quad de evaluacion de luces. */

	Texture m_gBufferAlbedoMetallicTexture;
	Texture m_gBufferAlbedoMetallicSRV;
	RenderTargetView m_gBufferAlbedoMetallicRTV;  /**< Target del MRT para Color y Metalizado. */

	Texture m_gBufferNormalRoughnessTexture;
	Texture m_gBufferNormalRoughnessSRV;
	RenderTargetView m_gBufferNormalRoughnessRTV; /**< Target del MRT para Vectores Normales y Rugosidad. */

	Texture m_gBufferWorldAoTexture;
	Texture m_gBufferWorldAoSRV;
	RenderTargetView m_gBufferWorldAoRTV;         /**< Target del MRT para Oclusion Ambiental y Posicion del mundo. */

	Texture m_gBufferEmissiveAlphaTexture;
	Texture m_gBufferEmissiveAlphaSRV;
	RenderTargetView m_gBufferEmissiveAlphaRTV;   /**< Target del MRT para Canal Alfa y pixeles Emisivos. */

	EditorViewportPass m_preShadowDebugPass;  /**< Captura especial opcional vinculada al editor GUI. */
	bool m_applyShadows = true;               /**< Switch que autoriza la multiplicacion del buffer de sombras. */
	unsigned int m_renderWidth = 1280;        /**< Variable que almacena el ancho actual del render viewport. */
	unsigned int m_renderHeight = 720;        /**< Variable que almacena la altura actual del render viewport. */

	CBPerFrame m_cbPerFrame{};                /**< Estructura C++ para empaquetado del CBuffer de Escena. */
	CBPerObject m_cbPerObject{};              /**< Estructura C++ para empaquetado del CBuffer Objeto. */
	CBPerMaterial m_cbPerMaterial{};          /**< Estructura C++ para empaquetado del CBuffer Material PBR. */

	/** @brief Estructura temporal usada para depuracion diferida en memoria. */
	struct DeferredLightingDebugData {
		int DebugViewMode = 0;
		float ShadowStrength = 1.0f;
		float pad0 = 0.0f;
		float pad1 = 0.0f;
	} m_lightingDebugData{};

	bool m_shadowFactorDebugEnabled = false;  /**< Activa un override visual sobre las colas de sombra generadas. */
	int m_deferredDebugViewMode = 0;          /**< Valor entero indexado con las salidas visuales de ImGui. */

	std::vector<const RenderObject*> m_opaqueQueue;      /**< Cola MRT diferida de modelos estandar. */
	std::vector<const RenderObject*> m_transparentQueue; /**< Cola Forward separada con reordenamiento para alpha blend. */
};