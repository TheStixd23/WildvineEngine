/**
 * @file BaseApp.h
 * @brief Declara la API de BaseApp dentro del subsistema Core.
 * @ingroup core
 */
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
#include "ECS/LightComponent.h"
#include "ECS/MeshRendererComponent.h"
#include "Rendering/Material.h"
#include "Rendering/MaterialInstance.h"
#include "Rendering/Mesh.h"
#include "Rendering/ForwardRenderer.h"
#include "Rendering/RenderScene.h"
#include <string>
extern IMGUI_IMPL_API
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**
 * @class BaseApp
 * @brief Coordina el ciclo de vida principal de Wildvine Engine.
 *
 * `BaseApp` inicializa la ventana, el dispositivo grafico, la interfaz del editor,
 * la escena de prueba y el pipeline de render. Tambien administra el bucle principal
 * de actualizacion, la serializacion basica de escena y la respuesta a cambios de tamano.
 */
class
	BaseApp {
public:
	BaseApp() = default;
	~BaseApp() { destroy(); }

	/**
	 * @brief Prepara subsistemas previos al render.
	 * @return `S_OK` si la aplicacion queda lista para continuar la inicializacion.
	 */
	HRESULT
		awake();

	/**
	 * @brief Ejecuta el bucle principal de la aplicacion.
	 * @param hInst Instancia Win32 actual.
	 * @param nCmdShow Modo inicial de visualizacion de la ventana.
	 * @return Codigo de salida del proceso.
	 */
	int
		run(HINSTANCE hInst, int nCmdShow);

	/**
	 * @brief Inicializa recursos graficos, escena, materiales y renderer.
	 * @return `S_OK` cuando todos los recursos base quedan listos.
	 */
	HRESULT
		init();

	/**
	 * @brief Ejecuta la logica por frame y sincroniza GUI, camara y escena.
	 * @param deltaTime Tiempo transcurrido desde el frame anterior.
	 */
	void
		update(float deltaTime);

	/**
	 * @brief Emite el frame actual en el viewport del editor y en el back buffer final.
	 */
	void
		render();

	/**
	 * @brief Libera recursos del motor en orden seguro de destruccion.
	 */
	void
		destroy();

	/**
	 * @brief Reconstuye recursos dependientes de la resolucion principal.
	 * @param newW Nuevo ancho del area cliente.
	 * @param newH Nuevo alto del area cliente.
	 */
	void
		onResize(unsigned int newW, unsigned int newH);

	/**
	 * @brief Atiende cambios diferidos del viewport interno del editor.
	 */
	void handleEditorViewportResize();

	/**
	 * @brief Serializa la escena actual a disco.
	 * @param path Ruta de salida del archivo `.wvscene`.
	 * @return `true` si la escena se guarda correctamente.
	 */
	bool saveScene(const std::string& path);

	/**
	 * @brief Carga una escena serializada previamente.
	 * @param path Ruta del archivo `.wvscene`.
	 * @return `true` si el contenido se pudo leer y aplicar.
	 */
	bool loadScene(const std::string& path);

	/**
	 * @brief Devuelve la ruta por defecto usada por el editor para persistencia rapida.
	 */
	std::string getDefaultScenePath() const;
private:
	/**
	 * @brief Callback de ventana estático para interceptar y despachar mensajes del SO.
	 * @param hWnd Handle de la ventana que recibe el mensaje.
	 * @param message Identificador del mensaje de sistema.
	 * @param wParam Información adicional específica del mensaje.
	 * @param lParam Información adicional específica del mensaje.
	 * @return Resultado del procesamiento del mensaje.
	 */
	static LRESULT CALLBACK
		WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


private:
	Window                              m_window;                 ///< Envoltorio de la ventana Win32.
	Device															m_device;                 ///< Representación del adaptador gráfico de hardware.
	DeviceContext										m_deviceContext;          ///< Contexto principal para emitir comandos de render.
	SwapChain                           m_swapChain;              ///< Cadena de intercambio para double/triple buffering.
	Texture                             m_backBuffer;             ///< Textura objetivo final en la ventana.
	RenderTargetView									  m_renderTargetView;       ///< Vista principal de render target hacia el back buffer.
	Texture                             m_depthStencil;           ///< Textura para prueba de profundidad y esténcil.
	DepthStencilView									  m_depthStencilView;       ///< Vista principal del buffer de profundidad.
	Viewport                            m_viewport;               ///< Área de dibujo definida sobre la ventana.
	ShaderProgram												m_shaderProgram;          ///< Programa base de shaders cargado.
	//Buffer															m_cbNeverChanges;
	//Buffer															m_cbChangeOnResize;
	bool m_d3dReady = false;                                      ///< Bandera que indica si Direct3D está inicializado.
	Buffer m_constantBuffer;                                      ///< Buffer constante principal general.
	CBMain m_constantBufferStruct;                                ///< Estructura de caché local para el buffer constante.

	// Textures
	Texture m_AlbedoSRV;                                          ///< Textura PBR: Color Base/Albedo.
	Texture m_MetallicSRV;                                        ///< Textura PBR: Metalicidad.
	Texture m_RoughnessSRV;                                       ///< Textura PBR: Rugosidad.
	Texture m_AOSRV;                                              ///< Textura PBR: Oclusión Ambiental.
	Texture m_NormalSRV;                                          ///< Textura PBR: Mapa de Normales.
	Texture m_EmissiveSRV;                                        ///< Textura PBR: Mapa Emisivo.
	Texture m_drakefireAlbedoSRV;                                 ///< Textura PBR específica de Drakefire: Albedo.
	Texture m_drakefireNormalSRV;                                 ///< Textura PBR específica de Drakefire: Normales.
	Texture m_drakefireMetallicSRV;                               ///< Textura PBR específica de Drakefire: Metalicidad.
	Texture m_drakefireRoughnessSRV;                              ///< Textura PBR específica de Drakefire: Rugosidad.
	Texture m_drakefireAOSRV;                                     ///< Textura PBR específica de Drakefire: AO.

	Camera															m_camera;                 ///< Cámara principal para renderizar la escena.

	SceneGraph												m_sceneGraph;               ///< Administrador jerárquico de entidades espaciales.
	std::vector<EU::TSharedPointer<Actor>> m_actors;              ///< Contenedor principal de actores en la escena.
	EU::TSharedPointer<Actor> m_cyberGun;                         ///< Actor de prueba: Arma Cyber.
	EU::TSharedPointer<Actor> m_drakefirePistol;                  ///< Actor de prueba: Pistola Drakefire.
	EU::TSharedPointer<Actor> m_directionalLightActor;            ///< Actor que contiene la componente de luz direccional.


	Model3D* m_model;                  ///< Puntero al modelo genérico de prueba cargado.
	Model3D* m_drakefireModel = nullptr; ///< Puntero al modelo Drakefire cargado.

	//CBChangeOnResize										cbChangesOnResize;
	//CBNeverChanges											cbNeverChanges;
	GUI																m_gui;                    ///< Instancia principal del sistema de interfaz (ImGui).
	bool m_guiInitialized = false;                                ///< Bandera que indica si el subsistema GUI está listo.
	EU::Vector3 m_cameraPos;                                      ///< Caché de la posición de la cámara (para UI u otros cálculos).

	Skybox m_skybox;                                              ///< Entorno de fondo de la escena (Skybox).
	Texture															m_skyboxTex;              ///< Cubemap utilizado para el Skybox.
	RasterizerState m_defaultRasterizer;                          ///< Estado de rasterizador estándar (culling, wireframe, etc.).
	DepthStencilState m_defaultDepthStencil;                      ///< Estado estándar de pruebas Z y esténcil.
	SamplerState m_defaultSampler;                                ///< Muestreador estándar para el filtrado de texturas.
	Mesh m_cyberGunRenderMesh;                                    ///< Malla procesada para el arma Cyber.
	Mesh m_drakefireRenderMesh;                                   ///< Malla procesada para el arma Drakefire.
	Material m_pbrMaterial;                                       ///< Material genérico configurado para PBR opaco.
	Material m_transparentPbrMaterial;                            ///< Material genérico configurado para PBR translúcido.
	MaterialInstance m_cyberGunMaterial;                          ///< Instancia PBR asignada al arma Cyber.
	MaterialInstance m_drakefireMaterial;                         ///< Instancia PBR asignada al arma Drakefire.

	EditorViewportPass m_editorViewportPass;                      ///< Textura objetivo y visor para la ventana del editor UI.
	ForwardRenderer m_forwardRenderer;                            ///< Instancia del sistema de renderizado base.
	RenderScene m_renderScene;                                    ///< Estructura temporal que agrupa datos visibles por frame.
	bool m_editorViewportResizePending = false;                   ///< Bandera que indica si el viewport del editor cambió de tamaño.
	unsigned int m_pendingViewportWidth = 1;                      ///< Ancho en espera para el próximo redimensionado del viewport.
	unsigned int m_pendingViewportHeight = 1;                     ///< Alto en espera para el próximo redimensionado del viewport.

	unsigned int m_lastRequestedViewportWidth = 1;                ///< Último ancho solicitado estable para evitar flickering.
	unsigned int m_lastRequestedViewportHeight = 1;               ///< Último alto solicitado estable para evitar flickering.
	int m_viewportResizeStableFrames = 0;                         ///< Contador de frames para confirmar estabilidad en la resolución del viewport.
};