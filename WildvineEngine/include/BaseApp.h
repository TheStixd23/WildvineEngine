#pragma once
#include "Prerequisites.h"
#include "Window.h"
#include "Device.h"
#include "DeviceContext.h"
#include "SwapChain.h"
#include "Texture.h"
#include "RenderTargetView.h"
#include "DepthStencilView.h"
#include "Viewport.h"
#include "ShaderProgram.h"
#include "MeshComponent.h"
#include "Buffer.h"
#include "SamplerState.h"
#include "Model3D.h"
#include "ECS/Actor.h"
#include "EngineUtilities\GUI/GUI.h"
#include "SceneGraph\SceneGraph.h"
#include "EngineUtilities\Utilities\Camera.h"
#include "EngineUtilities\Utilities\Skybox.h"
#include "EngineUtilities\Utilities\LayoutBuilder.h"
#include "EngineUtilities/Utilities/EditorViewportPass.h"

/**
 * @brief Declaración externa del manejador de mensajes de ImGui para Win32.
 * Necesario para delegar los eventos del teclado y ratón a la interfaz gráfica.
 */
extern IMGUI_IMPL_API
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**
 * @class   BaseApp
 * @brief   Clase principal que orquesta el ciclo de vida de la aplicación y del motor.
 *
 * @details La clase @c BaseApp actúa como el núcleo unificador de MonacoEngine3.
 * Se encarga de inicializar la ventana del sistema operativo, configurar el pipeline
 * gráfico de DirectX 11 (Device, SwapChain, Buffers), cargar los recursos iniciales,
 * y mantener el bucle principal de juego (Game Loop). Además, coordina el
 * @c SceneGraph, la interfaz de usuario (GUI) y la actualización de los actores.
 */
class
	BaseApp {
public:
	/**
	 * @brief Constructor por defecto.
	 */
	BaseApp() = default;

	/**
	 * @brief Destructor que garantiza la liberación de todos los recursos.
	 */
	~BaseApp() { destroy(); }

	/**
	 * @brief Configura la ventana del sistema antes de inicializar el entorno gráfico.
	 * @return Código @c HRESULT indicando el éxito o fallo de la creación de la ventana.
	 */
	HRESULT
		awake();

	/**
	 * @brief Inicia el bucle principal de mensajes y renderizado de la aplicación.
	 *
	 * Contiene el Game Loop que procesa mensajes de Windows y llama continuamente
	 * a @c update() y @c render() hasta que se recibe un mensaje de cierre.
	 *
	 * @param hInst   Manejador de la instancia de la aplicación proporcionado por Windows.
	 * @param nCmdShow Opciones de visualización inicial de la ventana (maximizado, minimizado, etc.).
	 * @return        Código de salida del sistema operativo al terminar la ejecución.
	 */
	int
		run(HINSTANCE hInst, int nCmdShow);

	/**
	 * @brief Inicializa los subsistemas gráficos y carga los recursos de la escena.
	 *
	 * Instancia el Device de DirectX, configura el SwapChain, los shaders, el
	 * grafo de escena, la interfaz gráfica y los modelos iniciales.
	 *
	 * @return Código @c HRESULT indicando el estado de la inicialización de DirectX.
	 */
	HRESULT
		init();

	/**
	 * @brief Lógica de actualización por frame del motor.
	 *
	 * Recoge entradas del usuario, actualiza la cámara, el SceneGraph y prepara
	 * los buffers constantes antes del renderizado.
	 *
	 * @param deltaTime Tiempo transcurrido en segundos desde el último frame.
	 */
	void
		update(float deltaTime);

	/**
	 * @brief Fase de renderizado del frame.
	 *
	 * Limpia los buffers, emite comandos de dibujo para la escena, renderiza el
	 * Skybox, la interfaz de usuario (GUI) y finalmente presenta el frame en pantalla.
	 */
	void
		render();

	/**
	 * @brief Libera ordenadamente los recursos gráficos y detiene los subsistemas.
	 */
	void
		destroy();

	/**
	 * @brief Recrea los buffers principales cuando la ventana de Windows cambia de tamaño.
	 *
	 * @param newW Nuevo ancho de la ventana.
	 * @param newH Nuevo alto de la ventana.
	 */
	void
		onResize(unsigned int newW, unsigned int newH);

	/**
	 * @brief Recrea los buffers del pase del editor si la ventana de ImGui fue redimensionada.
	 *
	 * Se encarga de aplicar la nueva resolución al @c EditorViewportPass asegurando que
	 * la escena dentro del editor conserve la relación de aspecto y resolución correcta.
	 */
	void handleEditorViewportResize();
private:
	/**
	 * @brief Manejador de eventos nativos de Windows (Window Procedure).
	 *
	 * Atrapa eventos del teclado, ratón y redimensionamiento de ventana, derivándolos
	 * a la interfaz de ImGui o a las funciones propias de la aplicación.
	 */
	static LRESULT CALLBACK
		WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


private:
	// ============================================================================
	// Subsistema Base y Gráficos (DirectX 11)
	// ============================================================================
	Window                              m_window;              ///< Envoltorio de la ventana de la API de Windows.
	Device								m_device;              ///< Dispositivo gráfico de creación de recursos.
	DeviceContext						m_deviceContext;       ///< Contexto para la emisión de comandos a la GPU.
	SwapChain                           m_swapChain;           ///< Cadena de intercambio para el doble/triple buffering.
	Texture                             m_backBuffer;          ///< Textura de color final mostrada en la ventana principal.
	RenderTargetView					m_renderTargetView;    ///< Vista para dibujar en el backbuffer principal.
	Texture                             m_depthStencil;        ///< Textura que almacena la información de profundidad.
	DepthStencilView					m_depthStencilView;    ///< Vista para evaluar qué píxeles están frente a otros.
	Viewport                            m_viewport;            ///< Área de mapeo del renderizado a la ventana.
	ShaderProgram						m_shaderProgram;       ///< Programa de shaders principal del motor.
	//Buffer							m_cbNeverChanges;
	//Buffer							m_cbChangeOnResize;
	bool m_d3dReady = false;                                   ///< Bandera que indica si DirectX 11 ha sido inicializado.
	Buffer m_constantBuffer;                                   ///< Buffer constante en VRAM para comunicación con Shaders.
	CBMain m_constantBufferStruct;                             ///< Estructura en RAM con los datos del Constant Buffer principal.

	// ============================================================================
	// Materiales y Texturas (PBR)
	// ============================================================================
	// Textures
	Texture m_AlbedoSRV;                                       ///< Textura de color base difuso (Albedo).
	Texture m_MetallicSRV;                                     ///< Textura que define las áreas metálicas.
	Texture m_RoughnessSRV;                                    ///< Textura que define la rugosidad/pulido de la superficie.
	Texture m_AOSRV;                                           ///< Textura de Oclusión Ambiental (Ambient Occlusion).
	Texture m_NormalSRV;                                       ///< Textura de Normales para relieves de alta definición.

	// ============================================================================
	// Escena y Entidades
	// ============================================================================
	Camera														m_camera;               ///< Cámara principal que visualiza la escena.

	SceneGraph													m_sceneGraph;           ///< Administrador global de jerarquías de entidades.
	std::vector<EU::TSharedPointer<Actor>> m_actors;									///< Colección plana de actores base instanciados en la escena.
	EU::TSharedPointer<Actor> m_cyberGun;												///< Puntero inteligente a un actor específico (ej. arma).


	Model3D* m_model;               ///< Recurso de modelo 3D cargado actualmente en memoria.

	//CBChangeOnResize										cbChangesOnResize;
	//CBNeverChanges											cbNeverChanges;
	// ============================================================================
	// UI y Entornos
	// ============================================================================
	GUI																m_gui;                  ///< Sistema de interfaz gráfica de usuario basado en ImGui.
	EU::Vector3 m_cameraPos;																///< Coordenadas actuales de la cámara (auxiliar).

	Skybox m_skybox;																		///< Entorno panorámico que proyecta el fondo de la escena.
	Texture															m_skyboxTex;            ///< Textura Cubemap asociada al Skybox.
	RasterizerState m_defaultRasterizer;													///< Estado de rasterización base del motor (Cull Back).
	DepthStencilState m_defaultDepthStencil;												///< Estado de prueba de profundidad base del motor (Less Equal).

	// ============================================================================
	// Pase de Viewport del Editor
	// ============================================================================
	EditorViewportPass m_editorViewportPass;                   ///< Pase de renderizado off-screen exclusivo para el visor de ImGui.
	bool m_editorViewportResizePending = false;                ///< Bandera que indica si el pase requiere recrear sus buffers.
	unsigned int m_pendingViewportWidth = 1;                   ///< Ancho temporal solicitado para el pase del editor.
	unsigned int m_pendingViewportHeight = 1;                  ///< Alto temporal solicitado para el pase del editor.

	unsigned int m_lastRequestedViewportWidth = 1;             ///< Último ancho estable solicitado por la ventana de ImGui.
	unsigned int m_lastRequestedViewportHeight = 1;            ///< Último alto estable solicitado por la ventana de ImGui.
	int m_viewportResizeStableFrames = 0;                      ///< Contador para retrasar la recreación de buffers hasta que el usuario termine de arrastrar la ventana.
};