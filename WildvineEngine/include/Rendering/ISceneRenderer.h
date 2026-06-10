/**
 * @file ISceneRenderer.h
 * @brief Declara una interfaz comun para los renderers de escena en WildvineEngine.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;
class Camera;
class RenderScene;
class EditorViewportPass;
//class Texture;

/**
 * @enum RenderType
 * @brief Define los tipos de pipeline de renderizado disponibles en el motor.
 */
enum class
    RenderType {
    Forward = 0,
    Deferred = 1
};

/**
 * @class ISceneRenderer
 * @brief Contrato base para cualquier renderer consumido por el pipeline principal.
 */
class
    ISceneRenderer {
public:
    virtual ~ISceneRenderer() = default;

    /**
     * @brief Inicializa los recursos del renderer.
     * @param device Referencia al dispositivo gráfico utilizado para crear los recursos.
     * @return HRESULT Código de éxito o error tras la inicialización.
     */
    virtual HRESULT init(Device& device) = 0;

    /**
     * @brief Reconstruye los recursos dependientes del tamaño de la pantalla o viewport.
     * @param device Referencia al dispositivo gráfico.
     * @param width Nuevo ancho en píxeles.
     * @param height Nuevo alto en píxeles.
     */
    virtual void resize(Device& device, unsigned int width, unsigned int height) = 0;

    /**
     * @brief Ejecuta el ciclo completo de renderizado de un frame.
     * @param deviceContext Contexto del dispositivo gráfico para emitir los comandos de dibujado.
     * @param camera Cámara activa desde la cual se está observando la escena.
     * @param scene Escena actual que contiene objetos y luces.
     * @param viewport Pase de destino final para el resultado del renderizado.
     */
    virtual void render(DeviceContext& deviceContext,
        const Camera& camera,
        RenderScene& scene,
        EditorViewportPass& viewport) = 0;

    /**
     * @brief Libera los recursos de GPU asociados a este renderer.
     */
    virtual void destroy() = 0;

    /**
     * @brief Obtiene la vista de recurso (SRV) del mapa de sombras principal.
     * @return ID3D11ShaderResourceView* Puntero al SRV, o nullptr si no es compatible.
     */
    virtual ID3D11ShaderResourceView* getShadowMapSRV() const { return nullptr; }

    /**
     * @brief Obtiene la vista de recurso (SRV) del pase de depuración de sombras.
     * @return ID3D11ShaderResourceView* Puntero al SRV, o nullptr si no es compatible.
     */
    virtual ID3D11ShaderResourceView* getPreShadowSRV() const { return nullptr; }

    /**
     * @brief Obtiene el SRV correspondiente al color base (Albedo) y la propiedad Metallic del G-Buffer.
     * @return ID3D11ShaderResourceView* Puntero al SRV, o nullptr si no es compatible.
     */
    virtual ID3D11ShaderResourceView* getGBufferAlbedoMetallicSRV() const { return nullptr; }

    /**
     * @brief Obtiene el SRV correspondiente a las Normales y la propiedad Roughness del G-Buffer.
     * @return ID3D11ShaderResourceView* Puntero al SRV, o nullptr si no es compatible.
     */
    virtual ID3D11ShaderResourceView* getGBufferNormalRoughnessSRV() const { return nullptr; }

    /**
     * @brief Obtiene el SRV correspondiente a la Posición en el Mundo (World) y la Oclusión Ambiental (AO) del G-Buffer.
     * @return ID3D11ShaderResourceView* Puntero al SRV, o nullptr si no es compatible.
     */
    virtual ID3D11ShaderResourceView* getGBufferWorldAoSRV() const { return nullptr; }

    /**
     * @brief Obtiene el SRV correspondiente al color Emisivo y el canal Alpha del G-Buffer.
     * @return ID3D11ShaderResourceView* Puntero al SRV, o nullptr si no es compatible.
     */
    virtual ID3D11ShaderResourceView* getGBufferEmissiveAlphaSRV() const { return nullptr; }

    /**
     * @brief Habilita o deshabilita la visualización de depuración del factor de sombras o shaders.
     * @param enabled True para habilitar la depuración, false para deshabilitarla.
     */
    virtual void setShaderFactorDebugEnabled(bool enabled) { (void)enabled; }

    /**
     * @brief Establece el modo de visualización de depuración para los componentes del G-Buffer.
     * @param mode Identificador entero del modo a visualizar (ej. Albedo, Normales, etc.).
     */
    virtual void setDeferredDebugViewMode(int mode) { (void)mode; }

    /**
     * @brief Devuelve el nombre identificador del renderer activo.
     * @return const char* Nombre del renderer.
     */
    virtual const char* getDebugName() const = 0;
};