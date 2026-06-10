/**
 * @file DeferredRenderer.h
 * @brief Declara la API de DeferredRenderer dentro del subsistema Rendering de WildvineEngine.
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
 * @brief Implementa el pipeline de renderizado diferido (Deferred Shading).
 * * Esta clase separa la evaluación de la geometría de la evaluación de la iluminación.
 * Primero almacena propiedades físicas y espaciales en un G-Buffer y posteriormente
 * resuelve la luz en un pase de pantalla completa, optimizando el costo en escenas
 * con múltiples fuentes de luz dinámica.
 */
class DeferredRenderer : public ISceneRenderer {
public:
    /**
     * @brief Inicializa los recursos base del motor de renderizado diferido.
     * @param device Dispositivo gráfico utilizado para crear buffers y estados.
     * @return HRESULT S_OK si se inicializa correctamente.
     */
    HRESULT init(Device& device) override;

    /**
     * @brief Redimensiona los Render Targets del G-Buffer al cambiar el tamaño de la ventana.
     * @param device Dispositivo gráfico.
     * @param width Nuevo ancho de resolución.
     * @param height Nuevo alto de resolución.
     */
    void resize(Device& device, unsigned int width, unsigned int height) override;

    /**
     * @brief Ejecuta la secuencia completa de dibujado (Shadows, Geometry, Lighting, Forward/Transparent).
     * @param deviceContext Contexto de dibujado de la API gráfica.
     * @param camera Cámara desde la cual se observa la escena.
     * @param scene Escena con los objetos opacos, transparentes y luces a procesar.
     * @param viewportPass Render Target de destino final para la salida del frame.
     */
    void render(DeviceContext& deviceContext,
        const Camera& camera,
        RenderScene& scene,
        EditorViewportPass& viewportPass) override;

    /**
     * @brief Libera los recursos de GPU asociados a texturas, buffers y estados.
     */
    void destroy() override;

    // =========================================================================
    // Getters para Shader Resource Views (SRV) utilizados en debug o post-procesado
    // =========================================================================

    ID3D11ShaderResourceView* getShadowMapSRV() const override { return m_shadowDepthSRV.m_textureFromImg; }
    ID3D11ShaderResourceView* getPreShadowSRV() const override { return m_preShadowDebugPass.getSRV(); }
    ID3D11ShaderResourceView* getGBufferAlbedoMetallicSRV() const override { return m_gBufferAlbedoMetallicSRV.m_textureFromImg; }
    ID3D11ShaderResourceView* getGBufferNormalRoughnessSRV() const override { return m_gBufferNormalRoughnessSRV.m_textureFromImg; }
    ID3D11ShaderResourceView* getGBufferWorldAoSRV() const override { return m_gBufferWorldAoSRV.m_textureFromImg; }
    ID3D11ShaderResourceView* getGBufferEmissiveAlphaSRV() const override { return m_gBufferEmissiveAlphaSRV.m_textureFromImg; }

    /**
     * @brief Habilita o deshabilita la visualización de la máscara de sombras por debug.
     */
    void setShadowFactorDebugEnabled(bool enabled) override { m_shadowFactorDebugEnabled = enabled; }

    /**
     * @brief Establece el modo de visualización de debug del G-Buffer (ej. ver solo Normales o Albedo).
     */
    void setDeferredDebugViewMode(int mode) override { m_deferredDebugViewMode = mode; }

    const char* getDebugName() const override { return "DeferredRenderer"; }

private:
    // =========================================================================
    // Lógica interna del Pipeline de Rendering
    // =========================================================================

    /** @brief Clasifica y ordena los objetos de la escena en colas (opaca y transparente). */
    void buildQueues(RenderScene& scene, const Camera& camera);
    /** @brief Actualiza el Constant Buffer principal con los datos de la cámara y de la escena actual. */
    void updatePerFrame(const Camera& camera, const RenderScene& scene, DeviceContext& deviceContext);
    /** @brief Recalcula las matrices de las luces direccionales para la proyección de sombras. */
    void updateLightMatrices(const Camera& camera, const RenderScene& scene);

    /** @brief Ejecuta el flujo principal de renderizado hacia un objetivo específico. */
    void renderSceneToTarget(DeviceContext& deviceContext, RenderScene& scene, EditorViewportPass& targetPass, bool applyShadows);

    /** @brief Asocia los múltiples Render Targets del G-Buffer al pipeline de salida. */
    void bindGBufferTargets(DeviceContext& deviceContext, ID3D11DepthStencilView* depthStencilView);
    /** @brief Asocia el Render Target de salida final. */
    void bindFinalTarget(DeviceContext& deviceContext, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthStencilView);
    /** @brief Desvincula las texturas del G-Buffer para que puedan ser leídas en el pase de luz. */
    void clearDeferredSRVs(DeviceContext& deviceContext);

    /** @brief Dibuja la geometría opaca llenando las texturas del G-Buffer. */
    void renderGeometryPass(DeviceContext& deviceContext);
    void renderGeometryObject(DeviceContext& deviceContext, const RenderObject& object);

    /** @brief Combina los datos del G-Buffer para resolver la ecuación de iluminación. */
    void renderLightingPass(DeviceContext& deviceContext);

    /** @brief Renderiza el Skybox como fondo de la escena. */
    void renderSkyboxPass(DeviceContext& deviceContext, RenderScene& scene);
    /** @brief Dibuja objetos transparentes utilizando Forward Rendering. */
    void renderTransparentPass(DeviceContext& deviceContext);
    void renderForwardObject(DeviceContext& deviceContext, const RenderObject& object, RenderPassType passType);

    /** @brief Genera el mapa de profundidad para calcular las sombras. */
    void renderShadowPass(DeviceContext& deviceContext);
    void renderShadowObject(DeviceContext& deviceContext, const RenderObject& object);

    // =========================================================================
    // Métodos de inicialización de recursos
    // =========================================================================

    HRESULT createShadowResources(Device& device);
    HRESULT createGBufferResources(Device& device, unsigned int width, unsigned int height);
    HRESULT createGBufferTarget(Device& device,
        unsigned int width,
        unsigned int height,
        DXGI_FORMAT format,
        Texture& texture,
        Texture& srv,
        RenderTargetView& rtv);
    HRESULT createLightingResources(Device& device);
    HRESULT createFullScreenQuad(Device& device);
    HRESULT createBlendStates(Device& device);
    ID3D11BlendState* resolveBlendState(const Material* material) const;

private:
    // =========================================================================
    // Constant Buffers
    // =========================================================================
    Buffer m_perFrameBuffer;       /**< Datos globales de la escena y cámara por frame. */
    Buffer m_perObjectBuffer;      /**< Datos de transformación (World Matrix) por objeto. */
    Buffer m_perMaterialBuffer;    /**< Propiedades físicas del material del objeto. */
    Buffer m_lightingDebugBuffer;  /**< Configuración de depuración para visualización de luz. */
    Buffer m_fullscreenVertexBuffer;
    Buffer m_fullscreenIndexBuffer;

    // =========================================================================
    // Estados de Pipeline (Depth & Blend)
    // =========================================================================
    DepthStencilState m_transparentDepthStencil;
    DepthStencilState m_disabledDepthStencil;
    DepthStencilState m_shadowDepthStencil;

    ID3D11BlendState* m_alphaBlendState = nullptr;
    ID3D11BlendState* m_opaqueBlendState = nullptr;
    ID3D11BlendState* m_additiveBlendState = nullptr;
    ID3D11BlendState* m_premultipliedBlendState = nullptr;
    float m_blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // =========================================================================
    // Recursos para el mapeo de sombras (Shadow Mapping)
    // =========================================================================
    Texture m_shadowDepthTexture;
    Texture m_shadowDepthSRV;
    DepthStencilView m_shadowDSV;
    ShaderProgram m_shadowShader;
    RasterizerState m_shadowRasterizer;
    unsigned int m_shadowMapSize = 2048; /**< Resolución base del Shadow Map. */

    // =========================================================================
    // Shaders y estados para la fase Deferred
    // =========================================================================
    ShaderProgram m_gBufferShader;
    ShaderProgram m_deferredLightingShader;
    SamplerState m_lightingSampler;
    RasterizerState m_fullscreenRasterizer;

    // =========================================================================
    // G-Buffer Render Targets y Texturas
    // =========================================================================
    Texture m_gBufferAlbedoMetallicTexture;
    Texture m_gBufferAlbedoMetallicSRV;
    RenderTargetView m_gBufferAlbedoMetallicRTV;

    Texture m_gBufferNormalRoughnessTexture;
    Texture m_gBufferNormalRoughnessSRV;
    RenderTargetView m_gBufferNormalRoughnessRTV;

    Texture m_gBufferWorldAoTexture;
    Texture m_gBufferWorldAoSRV;
    RenderTargetView m_gBufferWorldAoRTV;

    Texture m_gBufferEmissiveAlphaTexture;
    Texture m_gBufferEmissiveAlphaSRV;
    RenderTargetView m_gBufferEmissiveAlphaRTV;

    // =========================================================================
    // Variables de control y colas
    // =========================================================================
    EditorViewportPass m_preShadowDebugPass;
    bool m_applyShadows = true;
    unsigned int m_renderWidth = 1280;
    unsigned int m_renderHeight = 720;

    CBPerFrame m_cbPerFrame{};
    CBPerObject m_cbPerObject{};
    CBPerMaterial m_cbPerMaterial{};

    struct DeferredLightingDebugData {
        int DebugViewMode = 0;
        float ShadowStrength = 1.0f;
        float pad0 = 0.0f;
        float pad1 = 0.0f;
    } m_lightingDebugData{};

    bool m_shadowFactorDebugEnabled = false;
    int m_deferredDebugViewMode = 0;

    std::vector<const RenderObject*> m_opaqueQueue;      /**< Cola de renderizado para geometría opaca. */
    std::vector<const RenderObject*> m_transparentQueue; /**< Cola de renderizado para geometría transparente. */
};