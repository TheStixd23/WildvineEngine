#pragma once
#include "Prerequisites.h"

class Device;
class DeviceContext;
class Window;
class Texture;

/**
 * @class   SwapChain
 * @brief   Encapsula la cadena de intercambio (@c IDXGISwapChain) de DirectX 11.
 *
 * El Swap Chain es el componente de la infraestructura gráfica (DXGI)
 * responsable de presentar la imagen renderizada al usuario sin artefactos
 * visuales (como el *Screen Tearing*). Administra una colección de buffers
 * (típicamente un Front Buffer visible y uno o más Back Buffers ocultos).
 * La GPU dibuja en el Back Buffer y, al terminar, el Swap Chain intercambia
 * rápidamente los punteros para mostrar el nuevo frame en pantalla.
 *
 * Además, esta clase centraliza la configuración del muestreo múltiple
 * (**MSAA - Multisample Anti-Aliasing**) para suavizar los bordes de la geometría.
 */
class
    SwapChain {

public:
        /**
         * @brief Constructor por defecto.
         * @details Instancia la clase sin inicializar la infraestructura gráfica. Se requiere llamar a @c init().
         */
        SwapChain() = default;

        /**
         * @brief Destructor por defecto.
         * @warning Al igual que otras envolturas COM, no libera automáticamente la memoria de video.
         * Es obligatorio llamar a @c destroy() antes de que la instancia sea eliminada.
         */
        ~SwapChain() = default;

    // ============================================================================
    // Inicialización y Ciclo de Vida
    // ============================================================================

    /**
     * @brief Crea la cadena de intercambio y extrae el Back Buffer inicial.
     *
     * Configura las propiedades del Swap Chain (resolución, formato de píxel,
     * tasa de refresco y configuración MSAA) basándose en las dimensiones de la
     * ventana proporcionada. Posteriormente, extrae la textura 2D del Back Buffer
     * para que el motor pueda crear un Render Target sobre ella.
     *
     * @param device        Dispositivo gráfico de DirectX 11.
     * @param deviceContext Contexto del dispositivo para operaciones gráficas.
     * @param backBuffer    Objeto @c Texture donde se almacenará el puntero al Back Buffer extraído.
     * @param window        Ventana del sistema operativo (Win32) donde se presentará la imagen.
     * @return              Código @c HRESULT nativo indicando el éxito (@c S_OK) o el fallo.
     *
     * @post Si retorna @c S_OK, @c m_swapChain será válido y @c backBuffer contendrá la textura de destino.
     */
    HRESULT
            init(Device& device,
                 DeviceContext& deviceContext,
                 Texture& backBuffer,
                 Window window);

    /**
     * @brief Lógica de actualización del Swap Chain.
     *
     * Método de marcador arquitectónico. Útil para reaccionar a cambios de estado
     * a nivel de aplicación antes del renderizado (ej. transiciones a pantalla completa).
     *
     * @note Actualmente no realiza ninguna operación.
     */
    void
        update();

    /**
     * @brief Fase de renderizado.
     *
     * Método de marcador arquitectónico. El Swap Chain no procesa geometría ni emite
     * comandos de dibujo convencionales. Su función principal de visualización se realiza en @c present().
     *
     * @note Actualmente no realiza ninguna operación.
     */
    void
        render();

    /**
     * @brief Libera la cadena de intercambio y la infraestructura DXGI asociada.
     *
     * Llama a @c Release() sobre el Swap Chain, así como sobre las interfaces
     * subyacentes de DXGI (Device, Adapter y Factory) obtenidas durante la inicialización.
     *
     * @post @c m_swapChain apuntará a @c nullptr.
     */
    void
        destroy();

    // ============================================================================
    // Operaciones de Presentación y Manejo de Buffers
    // ============================================================================

    /**
     * @brief Intercambia el Back Buffer con el Front Buffer para mostrar el frame.
     *
     * Llama a @c IDXGISwapChain::Present(). Aquí se puede configurar el intervalo
     * de sincronización vertical (V-Sync). Un intervalo de 1 sincroniza la
     * presentación con la tasa de refresco del monitor, mientras que 0 permite
     * correr a los FPS máximos posibles.
     */
    void
        present();

    /**
     * @brief Ajusta el tamaño de los buffers internos del Swap Chain.
     *
     * Fundamental al redimensionar la ventana de Windows para evitar que la imagen
     * se estire o se distorsione.
     * @warning Antes de llamar a este método, todas las referencias al Back Buffer
     * (Render Target Views, texturas) deben ser liberadas.
     *
     * @param width  Nuevo ancho en píxeles.
     * @param height Nuevo alto en píxeles.
     * @return       Código @c HRESULT nativo de la operación.
     */
    HRESULT
           resizeBuffers(unsigned int width,
                         unsigned int height);

    /**
     * @brief Obtiene una referencia actualizada a la textura del Back Buffer.
     *
     * Normalmente se llama inmediatamente después de un @c resizeBuffers() para
     * volver a enlazar el Render Target principal.
     *
     * @param backBuffer Objeto @c Texture de destino donde se guardará la referencia.
     * @return           Código @c HRESULT nativo de la operación.
     */
    HRESULT
           getBackBuffer(Texture& backBuffer);

public:
        // ============================================================================
        // Interfaz Nativa
        // ============================================================================

        /** @brief Puntero COM principal a la cadena de intercambio de DXGI. */
        IDXGISwapChain* m_swapChain = nullptr;

        /** @brief Tipo de controlador activo (ej. Hardware, Referencia o WARP/Software). */
        D3D_DRIVER_TYPE m_driverType = D3D_DRIVER_TYPE_NULL;

private:
        // ============================================================================
        // Propiedades DXGI y MSAA
        // ============================================================================

        /** @brief Nivel máximo de características de DirectX soportado por la GPU actual. */
        D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;

        /** @brief Cantidad de muestras tomadas por píxel para el Anti-Aliasing (ej. 1 para sin AA, 4 para 4x MSAA). */
        unsigned int m_sampleCount;

        /** @brief Niveles de calidad soportados por el hardware para la configuración de @c m_sampleCount especificada. */
        unsigned int m_qualityLevels;

        // ============================================================================
        // Infraestructura de Direct Graphics (DXGI)
        // ============================================================================

        /** @brief Puntero a la interfaz DXGI del dispositivo, necesaria para comunicarse con los controladores de bajo nivel. */
        IDXGIDevice* m_dxgiDevice = nullptr;

        /** @brief Puntero a la interfaz del adaptador gráfico (la tarjeta de video física). */
        IDXGIAdapter* m_dxgiAdapter = nullptr;

        /** @brief Puntero a la fábrica DXGI utilizada para instanciar el Swap Chain. */
        IDXGIFactory* m_dxgiFactory = nullptr;
};