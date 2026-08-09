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
#include "GUI/GUI.h"
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
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderScene.h"
#include "Rendering/Frustum.h"
#include "Rendering/PerformanceProfiler.h"
#include "Rendering/Octree.h"
#include "CommandManager.h"
#include <string>
#include <array>
#include <unordered_map>
extern IMGUI_IMPL_API
LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

enum class ActorClipboardKind {
	Empty = 0,
	Model,
	Light
};

struct ActorClipboard {
	ActorClipboardKind kind = ActorClipboardKind::Empty;
	EU::TSharedPointer<Actor> sourceActor;
	EU::TSharedPointer<Actor> sourceParent;
	std::string name;
	std::string modelPath;
	EU::Vector3 position;
	EU::Vector3 rotation;
	EU::Vector3 scale = EU::Vector3(1.0f, 1.0f, 1.0f);
	LightData lightData{};
	bool actorActive = true;
	bool visible = true;
	bool castShadow = true;
	bool receiveShadow = true;
	bool selectable = true;
	bool followLightPosition = true;
	bool followLightDirection = false;
};

struct LoadedMaterialResources {
	std::string name;
	MaterialInstance materialInstance;
	Texture albedo;
	Texture normal;
	Texture metallic;
	Texture roughness;
	Texture ao;
	Texture emissive;

	void destroy() {
		albedo.destroy();
		normal.destroy();
		metallic.destroy();
		roughness.destroy();
		ao.destroy();
		emissive.destroy();
	}
};

struct LoadedModel {
	Mesh mesh;
	Material material;
	MaterialInstance materialInstance;
	Texture albedo, normal, metallic, roughness, ao, emissive;
	std::vector<std::unique_ptr<LoadedMaterialResources>> importedMaterials;
	EU::Vector3 localMin;
	EU::Vector3 localMax;

	// Copia CPU para picking preciso rayo-triangulo.
	std::vector<MeshComponent> cpuMeshes;
	std::string sourcePath;
};



struct GizmoEditState {
	EU::Vector3 position;
	EU::Vector3 rotation;
	EU::Vector3 scale;
};

class
	BaseApp {
public:
	BaseApp() = default;
	~BaseApp() { destroy(); }

	HRESULT
		awake();

	int
		run(HINSTANCE hInst, int nCmdShow);

	HRESULT
		init();

	void
		update(float deltaTime);

	void
		render();

	void
		destroy();

	void
		onResize(unsigned int newW, unsigned int newH);

	void handleEditorViewportResize();

	bool newScene();
	bool saveScene(const std::string& path);
	bool loadScene(const std::string& path);
	std::string getDefaultScenePath() const;

	void addActorToScene(const EU::TSharedPointer<Actor>& actor);
	void removeActorFromScene(const EU::TSharedPointer<Actor>& actor);
	bool reparentActor(
		const EU::TSharedPointer<Actor>& child,
		const EU::TSharedPointer<Actor>& parent,
		bool keepWorldTransform = true);

private:
	bool saveSceneInternal(const std::string& path, bool updateEditorState);
	bool loadSceneInternal(const std::string& path, bool recoveredAutosave);
	void clearCurrentScene();
	void createDefaultSceneLight();
	bool showOpenSceneDialog(std::string& outPath) const;
	bool showSaveSceneDialog(std::string& outPath) const;
	std::string getExecutableDirectory() const;
	std::string getAutosaveScenePath() const;
	std::string getSceneDisplayName() const;
	void deleteAutosaveFile();
	void updateSceneDirtyState();
	unsigned long long computeSceneSignature() const;
	bool fileExists(const std::string& path) const;
	bool ensureSceneDirectories() const;

	static LRESULT CALLBACK
		WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	Window                              m_window;
	Device															m_device;
	DeviceContext										m_deviceContext;
	SwapChain                           m_swapChain;
	Texture                             m_backBuffer;
	RenderTargetView									  m_renderTargetView;
	Texture                             m_depthStencil;
	DepthStencilView									  m_depthStencilView;
	Viewport                            m_viewport;
	ShaderProgram												m_shaderProgram;
	bool m_d3dReady = false;
	Buffer m_constantBuffer;
	CBMain m_constantBufferStruct;



	// --- Estado inicial para el boton Reset ---
	struct InitialTransform {
		EU::Vector3 position;
		EU::Vector3 rotation;
		EU::Vector3 scale;
	};
	bool m_initialStateCaptured = false;
	EU::Vector3 m_initialLightDir;
	EU::Vector3 m_initialLightColor;
	EU::Vector3 m_initialCameraPos;
	std::vector<InitialTransform> m_initialTransforms;
	void captureInitialState();
	void resetSceneToDefaults();
	void focusCameraOnActor(const EU::TSharedPointer<Actor>& actor);
	void fitCameraToScene();

	unsigned int m_lastDrawCalls = 0;
	Frustum m_cameraFrustum;
	Frustum m_debugFrustum;
	bool m_debugFrustumInitialized = false;
	PerformanceProfiler m_performanceProfiler;
	Octree m_octree;

	// Picking
	EU::Vector3 m_carModelLocalMin;
	EU::Vector3 m_carModelLocalMax;
	void pickActorFromMouse();
	bool getViewportRay(const ImVec2& screenPosition,
		XMFLOAT3& outOrigin,
		XMFLOAT3& outDirection) const;
	bool getViewportPlacementPosition(const ImVec2& screenPosition,
		const EU::Vector3& localMinimum,
		EU::Vector3& outPosition) const;


	CommandManager m_commands;
	bool m_prevGizmoUsing = false;
	bool m_gizmoEditing = false;
	int  m_gizmoEditActorIndex = -1;
	GizmoEditState m_gizmoBefore;
	bool captureGizmoState(int index, GizmoEditState& out);

	ActorClipboard m_clipboard;
	bool m_hasClipboard = false;

	EU::TSharedPointer<Actor> spawnCar(const std::string& name,
		const EU::Vector3& pos, const EU::Vector3& rot, const EU::Vector3& scale);

	EU::TSharedPointer<Actor> spawnActorFromSource(
		const std::string& modelPath,
		const std::string& name,
		const EU::Vector3& pos,
		const EU::Vector3& rot,
		const EU::Vector3& scale);

	EU::TSharedPointer<Actor> spawnLightActor(
		LightType type,
		const std::string& name,
		const EU::Vector3& position,
		const EU::Vector3& color,
		float intensity,
		float range,
		bool castShadow);

	void createStudioLightRig();
	void aimLightAtActor(
		const EU::TSharedPointer<Actor>& lightActor,
		const EU::TSharedPointer<Actor>& targetActor);
	EU::TSharedPointer<Actor> getSelectedActor() const;
	EU::TSharedPointer<Actor> findActorShared(Actor* actor) const;
	std::string makeUniqueActorName(
		const std::string& requestedName,
		const Actor* ignoredActor = nullptr) const;
	EU::TSharedPointer<Actor> cloneActor(
		const EU::TSharedPointer<Actor>& source,
		const std::string& newName,
		const EU::Vector3& newPosition);

	const std::vector<MeshComponent>* getActorCpuMeshes(
		const EU::TSharedPointer<Actor>& actor) const;

	void duplicateSelected();
	void deleteSelected();
	void copySelected();
	void pasteClipboard();
	void savePrefabSelected();
	void loadPrefab();

	std::vector<std::unique_ptr<LoadedModel>> m_loadedModels;
	std::vector<MeshComponent> m_carCpuMeshes;
	std::unordered_map<Actor*, std::string> m_actorSourcePaths;

	std::vector<Texture> m_thumbTextures;
	std::vector<AssetThumb> m_thumbnails;

	EU::TSharedPointer<Actor> loadModelActor(const std::string& modelPath);
	void loadModelTextures(
		LoadedModel& loadedModel,
		const Model3D& importedModel,
		const std::string& modelPath);
	void buildTextureThumbnails();

	bool getActorAABB(const EU::TSharedPointer<Actor>& actor, EU::Vector3& outMin, EU::Vector3& outMax);







	// Recursos del Alfa Romeo.
	static const size_t kCarTextureCount = 25;
	static const size_t kCarMaterialCount = 19;

	std::array<Texture, kCarTextureCount> m_carTextures;
	std::array<MaterialInstance, kCarMaterialCount> m_carMaterials;

	Camera m_camera;

	SceneGraph m_sceneGraph;
	std::vector<EU::TSharedPointer<Actor>> m_actors;
	EU::TSharedPointer<Actor> m_car01;
	EU::TSharedPointer<Actor> m_car02;
	EU::TSharedPointer<Actor> m_directionalLightActor;
	EU::TSharedPointer<Actor> m_lastSelectedRenderableActor;
	unsigned int m_lightNameCounter = 1;

	Model3D* m_carModel = nullptr;

	GUI m_gui;
	bool m_guiInitialized = false;

	std::string m_currentScenePath;
	bool m_sceneDirty = false;
	unsigned long long m_savedSceneSignature = 0ull;
	float m_autosaveTimer = 0.0f;
	bool m_recoveryAvailable = false;
	float m_autosaveIntervalSeconds = 30.0f;
	EU::Vector3 m_cameraPos;

	Skybox m_skybox;
	Texture m_skyboxTex;
	RasterizerState m_defaultRasterizer;
	DepthStencilState m_defaultDepthStencil;
	SamplerState m_defaultSampler;

	Mesh m_carRenderMesh;

	Material m_pbrMaterial;
	Material m_maskedPbrMaterial;
	Material m_transparentPbrMaterial;

	EditorViewportPass m_editorViewportPass;
	RenderPipeline m_renderPipeline;
	RenderScene m_renderScene;
	// Estado de orbita DCC (Alt + clic izquierdo).
	bool m_orbitActive = false;
	EU::Vector3 m_orbitPivot = EU::Vector3(0.0f, 0.0f, 0.0f);
	float m_orbitDistance = 5.0f;

	bool m_editorViewportResizePending = false;
	unsigned int m_pendingViewportWidth = 1;
	unsigned int m_pendingViewportHeight = 1;

	unsigned int m_lastRequestedViewportWidth = 1;
	unsigned int m_lastRequestedViewportHeight = 1;
	int m_viewportResizeStableFrames = 0;
};
