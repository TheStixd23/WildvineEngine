#pragma once
#include "Prerequisites.h"
#include "Texture.h"
#include "RenderTargetView.h"
#include "DepthStencilView.h"

class Device;
class DeviceContext;

/**
 * @class EditorViewportPass
 * @brief Gestiona el pase de renderizado dedicado a la ventana de previsualización (Viewport) del editor.
 *
 * Esta clase encapsula la lógica de "Render to Texture". Crea y administra un Render Target personalizado
 * y un Depth/Stencil buffer en la GPU. Esto permite redirigir el dibujado de la escena hacia una textura
 * en lugar de la ventana principal, para que el editor (e.g., ImGui) pueda mostrar la cámara del juego dentro de un panel.
 */
class EditorViewportPass {
public:
    /** @brief Constructor por defecto. */
    EditorViewportPass() = default;

    /** @brief Destructor por defecto. */
    ~EditorViewportPass() = default;

    /**
     * @brief Inicializa los recursos de renderizado asignando el tamaño inicial del Viewport.
     * @param device Referencia al dispositivo lógico de DirectX (creador de recursos).
     * @param width Ancho inicial en píxeles.
     * @param height Alto inicial en píxeles.
     * @return HRESULT Código de estado estándar de Windows (S_OK si tuvo éxito).
     */
    HRESULT init(Device& device, unsigned int width, unsigned int height);

    /**
     * @brief Modifica el tamaño de las texturas internas cuando el usuario escala la ventana del editor.
     * * Libera los buffers anteriores y recrea las texturas con las nuevas dimensiones para evitar distorsiones.
     * @param device Referencia al dispositivo gráfico.
     * @param width Nuevo ancho en píxeles.
     * @param height Nuevo alto en píxeles.
     * @return HRESULT S_OK si la redistribución de memoria en la GPU fue exitosa.
     */
    HRESULT resize(Device& device, unsigned int width, unsigned int height);

    /**
     * @brief Prepara el contexto gráfico para comenzar a dibujar en la textura de este pase.
     * * Enlaza el RTV (Color) y el DSV (Profundidad) al pipeline de renderizado y limpia sus buffers.
     * @param deviceContext Contexto de ejecución de comandos de la GPU.
     * @param clearColor Array de 4 flotantes (RGBA) que define el color de fondo de la limpieza (clear).
     */
    void begin(DeviceContext& deviceContext, const float clearColor[4]);

    /**
     * @brief Intercambia eficientemente los recursos internos con otra instancia (operación Swapping).
     * * Útil para técnicas de doble buffering o reutilización de pases sin costo de copia en memoria.
     * @param other Referencia al otro pase con el cual se intercambiarán los datos.
     */
    void swap(EditorViewportPass& other);

    /**
     * @brief Limpia exclusivamente el buffer de profundidad y stencil.
     * @param deviceContext Contexto de ejecución de comandos de la GPU.
     */
    void clearDepth(DeviceContext& deviceContext);

    /**
     * @brief Configura las dimensiones del área de visualización (Viewport) en el rasterizador de la GPU.
     * * Sincroniza el pipeline de hardware con el ancho y alto actuales de este pase.
     * @param deviceContext Contexto de ejecución de comandos de la GPU.
     */
    void setViewport(DeviceContext& deviceContext);

    /**
     * @brief Libera de forma explícita toda la memoria de video asignada a las texturas y vistas.
     */
    void destroy();

    /**
     * @brief Recupera la vista de recurso de shader (SRV) de la textura donde se renderizó la escena.
     * @note Este puntero es el que se le envía a la UI del editor (ej. ImGui::Image) para mostrar el juego en pantalla.
     * @return ID3D11ShaderResourceView* Puntero nativo de DirectX 11 a la textura de color.
     */
    ID3D11ShaderResourceView* getSRV() const { return m_colorSRV.m_textureFromImg; }

    /** @return unsigned int Ancho actual en píxeles del pase. */
    unsigned int getWidth() const { return m_width; }

    /** @return unsigned int Alto actual en píxeles del pase. */
    unsigned int getHeight() const { return m_height; }

    /**
     * @brief Verifica si los recursos de hardware subyacentes existen y son válidos para operar.
     * @return true Si las texturas y la vista de shader están correctamente inicializadas en la GPU.
     * @return false Si falta algún recurso crítico (posible fuga o fallo de inicialización).
     */
    bool isValid() const {
        return m_colorTexture.m_texture != nullptr &&
            m_colorSRV.m_textureFromImg != nullptr &&
            m_depthTexture.m_texture != nullptr;
    }

private:
    /**
     * @brief Método interno de asistencia para la creación y alojamiento de las texturas, RTV y DSV.
     */
    HRESULT createResources(Device& device, unsigned int width, unsigned int height);

private:
    /** @name Recursos de Color (Render Target) */
    ///@{
    Texture           m_colorTexture; ///< Textura nativa 2D donde se escriben los píxeles de color.
    Texture           m_colorSRV;     ///< Shader Resource View que permite leer la textura de color desde otros shaders o la UI.
    RenderTargetView  m_rtv;          ///< Render Target View que permite a la GPU utilizar la textura como buffer de salida de color.
    ///@}

    /** @name Recursos de Profundidad (Z-Buffer) */
    ///@{
    Texture           m_depthTexture; ///< Textura 2D de alta precisión que almacena la profundidad por píxel.
    DepthStencilView  m_dsv;          ///< Depth Stencil View necesaria para habilitar pruebas de profundidad (Z-Test) en este pase.
    ///@}

    unsigned int      m_width = 1;    ///< Resolución horizontal actual del Viewport.
    unsigned int      m_height = 1;   ///< Resolución vertical actual del Viewport.
};