#include "BaseApp.h"
#include "ResourceManager.h"
#include <fstream>
#include <iomanip>

namespace {

	static std::string toLowerCopy(std::string s) { for (char& c : s) if (c >= 'A' && c <= 'Z') c = (char)(c + 32); return s; }
	static std::string stripExt(const std::string& f) { size_t d = f.find_last_of('.'); return (d == std::string::npos) ? f : f.substr(0, d); }
	static std::string fileBaseName(const std::string& path) { size_t s = path.find_last_of("/\\"); std::string n = (s == std::string::npos) ? path : path.substr(s + 1); return stripExt(n); }
	static bool endsWith(const std::string& s, const std::string& suf) { return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0; }
	static bool containsStr(const std::string& s, const std::string& sub) { return s.find(sub) != std::string::npos; }
	static ExtensionType extFromName(const std::string& lower) { if (endsWith(lower, ".jpg") || endsWith(lower, ".jpeg")) return JPG; if (endsWith(lower, ".dds")) return DDS; return PNG; }
	static std::vector<std::string> listImageFiles(const std::string& dir) {
		std::vector<std::string> out; std::string pat = dir + "\\*"; WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA(pat.c_str(), &fd); if (h == INVALID_HANDLE_VALUE) return out;
		do {
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
			std::string lo = toLowerCopy(fd.cFileName);
			if (endsWith(lo, ".png") || endsWith(lo, ".jpg") || endsWith(lo, ".jpeg") || endsWith(lo, ".dds") || endsWith(lo, ".tga"))
				out.push_back(fd.cFileName);
		} while (FindNextFileA(h, &fd));
		FindClose(h); return out;
	}

	static bool rayTriangleIntersection(
		const XMFLOAT3& rayOrigin,
		const XMFLOAT3& rayDirection,
		const XMFLOAT3& v0,
		const XMFLOAT3& v1,
		const XMFLOAT3& v2,
		float& outDistance) {

		const float epsilon = 1e-7f;

		const float edge1x = v1.x - v0.x;
		const float edge1y = v1.y - v0.y;
		const float edge1z = v1.z - v0.z;

		const float edge2x = v2.x - v0.x;
		const float edge2y = v2.y - v0.y;
		const float edge2z = v2.z - v0.z;

		const float px = rayDirection.y * edge2z - rayDirection.z * edge2y;
		const float py = rayDirection.z * edge2x - rayDirection.x * edge2z;
		const float pz = rayDirection.x * edge2y - rayDirection.y * edge2x;

		const float determinant = edge1x * px + edge1y * py + edge1z * pz;
		if (fabsf(determinant) < epsilon) return false;

		const float inverseDeterminant = 1.0f / determinant;

		const float tx = rayOrigin.x - v0.x;
		const float ty = rayOrigin.y - v0.y;
		const float tz = rayOrigin.z - v0.z;

		const float u = (tx * px + ty * py + tz * pz) * inverseDeterminant;
		if (u < 0.0f || u > 1.0f) return false;

		const float qx = ty * edge1z - tz * edge1y;
		const float qy = tz * edge1x - tx * edge1z;
		const float qz = tx * edge1y - ty * edge1x;

		const float v =
			(rayDirection.x * qx +
			 rayDirection.y * qy +
			 rayDirection.z * qz) * inverseDeterminant;

		if (v < 0.0f || (u + v) > 1.0f) return false;

		const float distance =
			(edge2x * qx + edge2y * qy + edge2z * qz) *
			inverseDeterminant;

		if (distance <= epsilon) return false;

		outDistance = distance;
		return true;
	}

	static std::vector<std::string> listSubfolders(const std::string& dir) {
		std::vector<std::string> out; std::string pat = dir + "\\*"; WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA(pat.c_str(), &fd); if (h == INVALID_HANDLE_VALUE) return out;
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
			std::string n = fd.cFileName; if (n == "." || n == "..") continue;
			out.push_back(n);
		} while (FindNextFileA(h, &fd));
		FindClose(h); return out;
	}


	class TransformCommand : public ICommand {
	public:
		TransformCommand(EU::TSharedPointer<Actor> actor,
			const GizmoEditState& before,
			const GizmoEditState& after)
			: m_actor(actor), m_before(before), m_after(after) {
		}
		void undo() override { apply(m_before); }
		void redo() override { apply(m_after); }
		const char* name() const override { return "Transform"; }
	private:
		void apply(const GizmoEditState& s) {
			if (m_actor.isNull()) return;
			EU::TSharedPointer<Transform> t = m_actor->getComponent<Transform>();
			if (!t) return;
			t->setPosition(s.position);
			t->setRotation(s.rotation);
			t->setScale(s.scale);
		}
		EU::TSharedPointer<Actor> m_actor;
		GizmoEditState m_before;
		GizmoEditState m_after;
	};

	class SpawnActorCommand : public ICommand {
	public:
		SpawnActorCommand(BaseApp* app, EU::TSharedPointer<Actor> actor) : m_app(app), m_actor(actor) {}
		void undo() override { if (m_app) m_app->removeActorFromScene(m_actor); }
		void redo() override { if (m_app) m_app->addActorToScene(m_actor); }
		const char* name() const override { return "Spawn Actor"; }
	private:
		BaseApp* m_app;
		EU::TSharedPointer<Actor> m_actor;
	};

	class DeleteActorCommand : public ICommand {
	public:
		DeleteActorCommand(BaseApp* app, EU::TSharedPointer<Actor> actor) : m_app(app), m_actor(actor) {}
		void undo() override { if (m_app) m_app->addActorToScene(m_actor); }
		void redo() override { if (m_app) m_app->removeActorFromScene(m_actor); }
		const char* name() const override { return "Delete Actor"; }
	private:
		BaseApp* m_app;
		EU::TSharedPointer<Actor> m_actor;
	};

} // namespace

HRESULT
BaseApp::awake() {
	HRESULT hr = S_OK;
	m_sceneGraph.init();
	MESSAGE("Main", "Awake", "Application awake successfully.");
	return hr;
}

int
BaseApp::run(HINSTANCE hInst, int nCmdShow) {
	if (FAILED(m_window.init(hInst, nCmdShow, WndProc, this))) {
		ERROR("Main", "Run", "Failed to initialize window.");
		return 0;
	}
	if (FAILED(awake())) {
		ERROR("Main", "Run", "Failed to awake application.");
		return 0;
	}
	if (FAILED(init())) {
		ERROR("Main", "Run", "Failed to initialize device and device context.");
		return 0;
	}
	m_gui.init(m_window, m_device, m_deviceContext);
	m_guiInitialized = true;

	MSG msg = {};
	LARGE_INTEGER freq, prev;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&prev);
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			LARGE_INTEGER curr;
			QueryPerformanceCounter(&curr);
			float deltaTime = static_cast<float>(curr.QuadPart - prev.QuadPart) / freq.QuadPart;
			prev = curr;
			update(deltaTime);
			render();
		}
	}
	return (int)msg.wParam;
}

HRESULT
BaseApp::init() {
	HRESULT hr = S_OK;

	hr = m_swapChain.init(m_device, m_deviceContext, m_backBuffer, m_window);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", ("Failed SwapChain. HRESULT: " + std::to_string(hr)).c_str()); return hr; }

	hr = m_renderTargetView.init(m_device, m_backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed RTV."); return hr; }

	hr = m_depthStencil.init(m_device, m_window.m_width, m_window.m_height,
		DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL, 4, 0);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed DepthStencil."); return hr; }

	hr = m_depthStencilView.init(m_device, m_depthStencil, DXGI_FORMAT_D24_UNORM_S8_UINT);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed DSV."); return hr; }

	hr = m_viewport.init(m_window);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Viewport."); return hr; }
	m_d3dReady = true;

	std::array<std::string, 6> faces = {
		"Skybox/cubemap_0.png", "Skybox/cubemap_1.png", "Skybox/cubemap_2.png",
		"Skybox/cubemap_3.png", "Skybox/cubemap_4.png", "Skybox/cubemap_5.png"
	};
	m_skyboxTex.CreateCubemap(m_device, m_deviceContext, faces, false);

	// ---- Cargar modelo Rana + texturas PBR ----
	std::vector<MeshComponent> ranaMeshes;
	m_ranaModel = new Model3D("Assets/Models/Rana.fbx", ModelType::FBX);
	ranaMeshes = m_ranaModel->GetMeshes();

	if (ranaMeshes.empty()) {
		ERROR("Main", "InitDevice", "Rana.fbx no contiene mallas o no pudo cargarse.");
		return E_FAIL;
	}

	// Cuerpo
	hr = m_ranaBodyAlbedo.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Body_BC", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Body Albedo."); return hr; }
	hr = m_ranaBodyMetallic.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Body_M", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Body Metallic."); return hr; }
	hr = m_ranaBodyRoughness.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Body_R", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Body Roughness."); return hr; }
	hr = m_ranaBodyAO.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Body_AO", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Body AO."); return hr; }
	hr = m_ranaBodyNormal.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Body_N", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Body Normal."); return hr; }

	// Cabeza
	hr = m_ranaHeadAlbedo.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Head_BC", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Head Albedo."); return hr; }
	hr = m_ranaHeadRoughness.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Head_R", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Head Roughness."); return hr; }
	hr = m_ranaHeadAO.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Head_AO", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Head AO."); return hr; }
	hr = m_ranaHeadNormal.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Head_N", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Head Normal."); return hr; }

	// Cristal. Se mantiene opaco por ahora porque el renderer clasifica
	// la transparencia por actor completo, no por submalla.
	hr = m_ranaGlassAlbedo.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Glass_BC", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Glass Albedo."); return hr; }
	hr = m_ranaGlassRoughness.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Glass_R", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Glass Roughness."); return hr; }
	hr = m_ranaGlassAO.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Glass_O", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Glass Occlusion."); return hr; }
	hr = m_ranaGlassNormal.init(m_device, "Assets/Textures/Rana/Sci-FIToad_Glass_N", PNG);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana Glass Normal."); return hr; }

	// ---- Mesh de render compartido por las dos instancias ----
	m_ranaRenderMesh.destroy();
	for (const MeshComponent& meshComponent : ranaMeshes) {
		Submesh submesh{};
		hr = submesh.vertexBuffer.init(m_device, meshComponent, D3D11_BIND_VERTEX_BUFFER);
		if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana vertex buffer."); return hr; }
		hr = submesh.indexBuffer.init(m_device, meshComponent, D3D11_BIND_INDEX_BUFFER);
		if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rana index buffer."); return hr; }
		submesh.indexCount = meshComponent.m_numIndex;
		submesh.startIndex = 0;

		// 0 = Body, 1 = Head, 2 = Glass.
		// El slot se decide usando el nombre de la malla dentro del FBX.
		std::string meshName = toLowerCopy(meshComponent.m_name);
		if (containsStr(meshName, "glass") || containsStr(meshName, "visor")) {
			submesh.materialSlot = 2;
		}
		else if (containsStr(meshName, "head") || containsStr(meshName, "cabeza")) {
			submesh.materialSlot = 1;
		}
		else {
			submesh.materialSlot = 0;
		}

		m_ranaRenderMesh.getSubmeshes().push_back(std::move(submesh));
	}

	// AABB local del modelo (para picking)
	m_ranaModelLocalMin = EU::Vector3(1e9f, 1e9f, 1e9f);
	m_ranaModelLocalMax = EU::Vector3(-1e9f, -1e9f, -1e9f);
	for (const MeshComponent& mc : ranaMeshes) {
		for (const SimpleVertex& v : mc.m_vertex) {
			m_ranaModelLocalMin.x = fminf(m_ranaModelLocalMin.x, v.Position.x);
			m_ranaModelLocalMin.y = fminf(m_ranaModelLocalMin.y, v.Position.y);
			m_ranaModelLocalMin.z = fminf(m_ranaModelLocalMin.z, v.Position.z);
			m_ranaModelLocalMax.x = fmaxf(m_ranaModelLocalMax.x, v.Position.x);
			m_ranaModelLocalMax.y = fmaxf(m_ranaModelLocalMax.y, v.Position.y);
			m_ranaModelLocalMax.z = fmaxf(m_ranaModelLocalMax.z, v.Position.z);
		}
	}

	LayoutBuilder builder;
	builder.Add("POSITION", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("BITANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT);
	hr = m_shaderProgram.init(m_device, "PBRShader.hlsl", builder);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed ShaderProgram."); return hr; }

	hr = m_constantBuffer.init(m_device, sizeof(CBMain));
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed constant buffer."); return hr; }

	m_camera.setLens(XM_PIDIV4, m_window.m_width / (float)m_window.m_height, 0.01f, 100.0f);
	m_camera.setPosition(0.0f, 3.0f, -6.0f);

	m_constantBufferStruct.LightColor = EU::Vector3(1.0f, 1.0f, 1.0f);
	m_constantBufferStruct.LightDir = EU::Vector3(-0.20f, -1.0f, 1.0f);

	m_skybox.init(m_device, &m_deviceContext, m_skyboxTex);

	hr = m_defaultRasterizer.init(m_device, D3D11_FILL_SOLID, D3D11_CULL_NONE, false, true);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Rasterizer."); return hr; }
	hr = m_defaultDepthStencil.init(m_device, true, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed DepthStencilState."); return hr; }
	hr = m_defaultSampler.init(m_device);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed SamplerState."); return hr; }

	m_pbrMaterial.setShader(&m_shaderProgram);
	m_pbrMaterial.setRasterizerState(&m_defaultRasterizer);
	m_pbrMaterial.setDepthStencilState(&m_defaultDepthStencil);
	m_pbrMaterial.setSamplerState(&m_defaultSampler);
	m_pbrMaterial.setDomain(MaterialDomain::Opaque);
	m_pbrMaterial.setBlendMode(BlendMode::Opaque);

	// ---- Material del cuerpo ----
	m_ranaBodyMaterial.setMaterial(&m_pbrMaterial);
	m_ranaBodyMaterial.setAlbedo(&m_ranaBodyAlbedo);
	m_ranaBodyMaterial.setNormal(&m_ranaBodyNormal);
	m_ranaBodyMaterial.setMetallic(&m_ranaBodyMetallic);
	m_ranaBodyMaterial.setRoughness(&m_ranaBodyRoughness);
	m_ranaBodyMaterial.setAO(&m_ranaBodyAO);
	m_ranaBodyMaterial.getParams().baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_ranaBodyMaterial.getParams().metallic = 0.0f;
	m_ranaBodyMaterial.getParams().roughness = 0.55f;
	m_ranaBodyMaterial.getParams().ao = 1.0f;
	m_ranaBodyMaterial.getParams().normalScale = 1.0f;
	m_ranaBodyMaterial.getParams().emissiveStrength = 0.0f;
	m_ranaBodyMaterial.getParams().alphaCutoff = 0.5f;

	// ---- Material de la cabeza ----
	m_ranaHeadMaterial.setMaterial(&m_pbrMaterial);
	m_ranaHeadMaterial.setAlbedo(&m_ranaHeadAlbedo);
	m_ranaHeadMaterial.setNormal(&m_ranaHeadNormal);
	m_ranaHeadMaterial.setMetallic(&m_ranaBodyMetallic); // fallback: no hay mapa M de cabeza
	m_ranaHeadMaterial.setRoughness(&m_ranaHeadRoughness);
	m_ranaHeadMaterial.setAO(&m_ranaHeadAO);
	m_ranaHeadMaterial.getParams().baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_ranaHeadMaterial.getParams().metallic = 0.0f;
	m_ranaHeadMaterial.getParams().roughness = 0.60f;
	m_ranaHeadMaterial.getParams().ao = 1.0f;
	m_ranaHeadMaterial.getParams().normalScale = 1.0f;
	m_ranaHeadMaterial.getParams().emissiveStrength = 0.0f;
	m_ranaHeadMaterial.getParams().alphaCutoff = 0.5f;

	// ---- Material del cristal ----
	m_ranaGlassMaterial.setMaterial(&m_pbrMaterial);
	m_ranaGlassMaterial.setAlbedo(&m_ranaGlassAlbedo);
	m_ranaGlassMaterial.setNormal(&m_ranaGlassNormal);
	m_ranaGlassMaterial.setMetallic(&m_ranaBodyMetallic); // fallback
	m_ranaGlassMaterial.setRoughness(&m_ranaGlassRoughness);
	m_ranaGlassMaterial.setAO(&m_ranaGlassAO);
	m_ranaGlassMaterial.getParams().baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_ranaGlassMaterial.getParams().metallic = 0.0f;
	m_ranaGlassMaterial.getParams().roughness = 0.20f;
	m_ranaGlassMaterial.getParams().ao = 1.0f;
	m_ranaGlassMaterial.getParams().normalScale = 1.0f;
	m_ranaGlassMaterial.getParams().emissiveStrength = 0.0f;
	m_ranaGlassMaterial.getParams().alphaCutoff = 0.5f;

	// ---- Actor 1 ----
	m_rana01 = EU::MakeShared<Actor>(m_device);
	if (m_rana01.isNull()) { ERROR("Main", "InitDevice", "Failed actor 1."); return E_FAIL; }
		m_rana01->setName("Rana_01");
	m_rana01->getComponent<Transform>()->setTransform(
		EU::Vector3(0.0f, 0.0f, 5.60f),
		EU::Vector3(0.0f, 0.0f, 0.0f),
		EU::Vector3(0.10f, 0.10f, 0.10f));
	{
		EU::TSharedPointer<MeshRendererComponent> mr = m_rana01->getComponent<MeshRendererComponent>();
		if (!mr) { mr = EU::MakeShared<MeshRendererComponent>(); m_rana01->addComponent(mr); }
		mr->setMesh(&m_ranaRenderMesh);
		mr->setMaterialInstances({ &m_ranaBodyMaterial, &m_ranaHeadMaterial, &m_ranaGlassMaterial });
		mr->setVisible(true);
		mr->setCastShadow(true);
	}
	m_actors.push_back(m_rana01);
	m_sceneGraph.addEntity(m_rana01.get());
	m_actorSourcePaths[m_rana01.get()] = "Assets/Models/Rana.fbx";

	// ---- Actor 2 desactivado temporalmente ----
	// Se deja una sola rana visible para evitar que dos instancias se encimen.
	m_rana02 = EU::TSharedPointer<Actor>();

	// ---- Luz direccional ----
	m_directionalLightActor = EU::MakeShared<Actor>(m_device);
	if (!m_directionalLightActor.isNull()) {
		m_directionalLightActor->setName("Directional Light");
		EU::TSharedPointer<LightComponent> lightComponent = m_directionalLightActor->getComponent<LightComponent>();
		if (!lightComponent) { lightComponent = EU::MakeShared<LightComponent>(); m_directionalLightActor->addComponent(lightComponent); }
		lightComponent->getLightData().type = LightType::Directional;
		lightComponent->getLightData().direction = m_constantBufferStruct.LightDir;
		lightComponent->getLightData().color = m_constantBufferStruct.LightColor;
		lightComponent->getLightData().intensity = 1.0f;
		lightComponent->getLightData().range = 12.0f;
		lightComponent->setCastShadow(true);
		m_actors.push_back(m_directionalLightActor);
		m_sceneGraph.addEntity(m_directionalLightActor.get());
	}

	hr = m_editorViewportPass.init(m_device, 1280, 720);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed EditorViewportPass."); return hr; }

	hr = m_renderPipeline.init(m_device, RendererType::Deferred);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed RenderPipeline."); return hr; }

	buildTextureThumbnails();


	return S_OK;
}

void
BaseApp::update(float deltaTime) {
	handleEditorViewportResize();

	if (!m_initialStateCaptured) {
		captureInitialState();
		m_initialStateCaptured = true;
	}

	// GUI
	m_gui.update(m_viewport, m_window);
	m_gui.drawViewportPanel(m_editorViewportPass.getSRV());
	m_gui.drawViewportGrid(m_camera);

	if (!m_actors.empty() && m_gui.selectedActorIndex >= 0 &&
		m_gui.selectedActorIndex < (int)m_actors.size()) {
		m_gui.inspectorGeneral(m_actors[m_gui.selectedActorIndex]);
		m_gui.editTransform(m_camera, m_window, m_actors[m_gui.selectedActorIndex]);
	}
	m_gui.outliner(m_actors);

	m_gui.drawGBufferDebugPanel(
		m_renderPipeline.getGBufferAlbedoMetallicSRV(),
		m_renderPipeline.getGBufferNormalRoughnessSRV(),
		m_renderPipeline.getGBufferWorldAoSRV(),
		m_renderPipeline.getGBufferEmissiveAlphaSRV());
	m_gui.drawRenderDebugPanel(
		m_renderPipeline.getPreShadowSRV(),
		m_editorViewportPass.getSRV(),
		m_renderPipeline.getShadowMapSRV());
	m_gui.drawLightingPanel(&m_constantBufferStruct.LightDir.x, &m_constantBufferStruct.LightColor.x);
	m_gui.drawStatsPanel(deltaTime, m_lastDrawCalls);
	m_gui.drawTexturePreview();
	m_gui.drawConsolePanel();
	m_gui.drawContentBrowser(m_thumbnails);

	// Instanciar modelo desde el Content Browser
	if (m_gui.m_assetSpawnRequested) {
		m_gui.m_assetSpawnRequested = false;
		EU::TSharedPointer<Actor> a = loadModelActor(m_gui.m_assetSpawnPath);
		if (!a.isNull()) {
			addActorToScene(a);
			m_commands.push(std::unique_ptr<ICommand>(new SpawnActorCommand(this, a)));
			m_gui.selectedActorIndex = (int)m_actors.size() - 1;
			MESSAGE("BaseApp", "loadModelActor", "Modelo instanciado desde Content");
		}
	}


	if (m_gui.consumeResetRequest()) {
		resetSceneToDefaults();
	}

	// --- Undo/Redo: registrar movimientos del gizmo ---
	{
		bool usingGizmo = m_gui.m_isUsingGizmo;
		int sel = m_gui.selectedActorIndex;

		if (usingGizmo && !m_prevGizmoUsing) {
			m_gizmoEditActorIndex = sel;
			m_gizmoEditing = captureGizmoState(sel, m_gizmoBefore);
		}
		else if (!usingGizmo && m_prevGizmoUsing && m_gizmoEditing) {
			GizmoEditState after;
			if (captureGizmoState(m_gizmoEditActorIndex, after)) {
				const float eps = 1e-4f;
				bool changed =
					fabsf(after.position.x - m_gizmoBefore.position.x) > eps ||
					fabsf(after.position.y - m_gizmoBefore.position.y) > eps ||
					fabsf(after.position.z - m_gizmoBefore.position.z) > eps ||
					fabsf(after.rotation.x - m_gizmoBefore.rotation.x) > eps ||
					fabsf(after.rotation.y - m_gizmoBefore.rotation.y) > eps ||
					fabsf(after.rotation.z - m_gizmoBefore.rotation.z) > eps ||
					fabsf(after.scale.x - m_gizmoBefore.scale.x) > eps ||
					fabsf(after.scale.y - m_gizmoBefore.scale.y) > eps ||
					fabsf(after.scale.z - m_gizmoBefore.scale.z) > eps;
				if (changed && m_gizmoEditActorIndex >= 0 &&
					m_gizmoEditActorIndex < (int)m_actors.size()) {
					EU::TSharedPointer<Actor> actor = m_actors[m_gizmoEditActorIndex];
					m_commands.push(std::unique_ptr<ICommand>(
						new TransformCommand(actor, m_gizmoBefore, after)));
				}
			}
			m_gizmoEditing = false;
		}
		m_prevGizmoUsing = usingGizmo;
	}

	// --- Atajos Undo/Redo ---
	{
		ImGuiIO& io = ImGui::GetIO();
		bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
		static bool zPrev = false, yPrev = false;
		bool zNow = (GetAsyncKeyState('Z') & 0x8000) != 0;
		bool yNow = (GetAsyncKeyState('Y') & 0x8000) != 0;

		bool menuUndo = m_gui.consumeUndoRequest();
		bool menuRedo = m_gui.consumeRedoRequest();
		bool doUndo = menuUndo || (ctrl && zNow && !zPrev && !io.WantTextInput);
		bool doRedo = menuRedo || (ctrl && yNow && !yPrev && !io.WantTextInput);

		if (doUndo) { m_commands.undo(); MESSAGE("BaseApp", "undo", "Deshacer"); }
		if (doRedo) { m_commands.redo(); MESSAGE("BaseApp", "redo", "Rehacer"); }

		zPrev = zNow; yPrev = yNow;
	}

	// --- Atajos Duplicar / Copiar / Pegar / Borrar / Prefab ---
	{
		ImGuiIO& io = ImGui::GetIO();
		bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
		bool typing = io.WantTextInput;
		static bool dPrev = false, cPrev = false, vPrev = false, delPrev = false;
		bool dNow = (GetAsyncKeyState('D') & 0x8000) != 0;
		bool cNow = (GetAsyncKeyState('C') & 0x8000) != 0;
		bool vNow = (GetAsyncKeyState('V') & 0x8000) != 0;
		bool delNow = (GetAsyncKeyState(VK_DELETE) & 0x8000) != 0;

		bool doDup = (!typing && ctrl && dNow && !dPrev) || m_gui.m_duplicateRequested;
		bool doCopy = (!typing && ctrl && cNow && !cPrev) || m_gui.m_copyRequested;
		bool doPaste = (!typing && ctrl && vNow && !vPrev) || m_gui.m_pasteRequested;
		bool doDel = (!typing && delNow && !delPrev) || m_gui.m_deleteRequested;
		bool doSave = m_gui.m_savePrefabRequested;
		bool doLoad = m_gui.m_loadPrefabRequested;

		m_gui.m_duplicateRequested = false;
		m_gui.m_copyRequested = false;
		m_gui.m_pasteRequested = false;
		m_gui.m_deleteRequested = false;
		m_gui.m_savePrefabRequested = false;
		m_gui.m_loadPrefabRequested = false;

		if (doCopy) { MESSAGE("BaseApp", "input", "Copy detectado");      copySelected(); }
		if (doDup) { MESSAGE("BaseApp", "input", "Duplicate detectado"); duplicateSelected(); }
		if (doPaste) { MESSAGE("BaseApp", "input", "Paste detectado");     pasteClipboard(); }
		if (doDel) { MESSAGE("BaseApp", "input", "Delete detectado");    deleteSelected(); }
		if (doSave)  savePrefabSelected();
		if (doLoad)  loadPrefab();

		dPrev = dNow; cPrev = cNow; vPrev = vNow; delPrev = delNow;
	}

	// --- Navegacion de camara estilo Unreal / DCC ---
	// Clic derecho + mouse: mirar.
	// WASD + Q/E: desplazamiento libre.
	// Alt + clic izquierdo: orbitar alrededor del objeto seleccionado.
	// Clic central: pan.
	// Rueda: zoom; clic derecho + rueda: velocidad.
	if (!m_gui.m_isUsingGizmo) {
		ImGuiIO& io = ImGui::GetIO();

		if (m_gui.m_viewportHovered && !io.WantTextInput) {
			const bool ctrlPressed =
				(GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			const bool altPressed =
				(GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			const bool rightMouseDown =
				ImGui::IsMouseDown(ImGuiMouseButton_Right);
			const bool middleMouseDown =
				ImGui::IsMouseDown(ImGuiMouseButton_Middle);
			const bool leftMouseDown =
				ImGui::IsMouseDown(ImGuiMouseButton_Left);

			static float cameraMovementSpeed = 4.0f;

			if (rightMouseDown && io.MouseWheel != 0.0f) {
				cameraMovementSpeed += io.MouseWheel * 0.75f;
				if (cameraMovementSpeed < 1.0f) cameraMovementSpeed = 1.0f;
				if (cameraMovementSpeed > 30.0f) cameraMovementSpeed = 30.0f;
			}
			else if (!rightMouseDown && io.MouseWheel != 0.0f) {
				m_camera.walk(io.MouseWheel * 0.70f);
			}

			// Orbita DCC: Alt + clic izquierdo.
			if (altPressed && leftMouseDown) {
				if (!m_orbitActive) {
					m_orbitActive = true;

					bool pivotFound = false;
					if (m_gui.selectedActorIndex >= 0 &&
						m_gui.selectedActorIndex < (int)m_actors.size()) {

						EU::TSharedPointer<Actor> selected =
							m_actors[m_gui.selectedActorIndex];

						EU::Vector3 localMin, localMax;
						EU::TSharedPointer<Transform> transform =
							selected.isNull()
							? EU::TSharedPointer<Transform>()
							: selected->getComponent<Transform>();

						if (transform && getActorAABB(selected, localMin, localMax)) {
							XMVECTOR localCenter = XMVectorSet(
								(localMin.x + localMax.x) * 0.5f,
								(localMin.y + localMax.y) * 0.5f,
								(localMin.z + localMax.z) * 0.5f,
								1.0f);

							XMVECTOR worldCenter =
								XMVector3TransformCoord(localCenter, transform->worldMatrix);

							XMFLOAT3 center;
							XMStoreFloat3(&center, worldCenter);
							m_orbitPivot = EU::Vector3(center.x, center.y, center.z);
							pivotFound = true;
						}
					}

					if (!pivotFound) {
						EU::Vector3 position = m_camera.getPosition();
						EU::Vector3 forward = m_camera.GetForward();
						m_orbitPivot = EU::Vector3(
							position.x + forward.x * 5.0f,
							position.y + forward.y * 5.0f,
							position.z + forward.z * 5.0f);
					}

					EU::Vector3 position = m_camera.getPosition();
					const float dx = position.x - m_orbitPivot.x;
					const float dy = position.y - m_orbitPivot.y;
					const float dz = position.z - m_orbitPivot.z;
					m_orbitDistance = sqrtf(dx * dx + dy * dy + dz * dz);
					if (m_orbitDistance < 0.25f) m_orbitDistance = 0.25f;
				}

				ImGui::SetMouseCursor(ImGuiMouseCursor_None);

				EU::Vector3 position = m_camera.getPosition();
				XMVECTOR offset = XMVectorSet(
					position.x - m_orbitPivot.x,
					position.y - m_orbitPivot.y,
					position.z - m_orbitPivot.z,
					0.0f);

				const float orbitSensitivity = 0.0040f;

				XMMATRIX yawRotation =
					XMMatrixRotationY(-io.MouseDelta.x * orbitSensitivity);
				offset = XMVector3TransformNormal(offset, yawRotation);

				EU::Vector3 cameraRight = m_camera.GetRight();
				XMVECTOR rightAxis = XMVector3Normalize(XMVectorSet(
					cameraRight.x, cameraRight.y, cameraRight.z, 0.0f));

				XMMATRIX pitchRotation = XMMatrixRotationAxis(
					rightAxis,
					-io.MouseDelta.y * orbitSensitivity);
				offset = XMVector3TransformNormal(offset, pitchRotation);

				offset = XMVector3Normalize(offset) * m_orbitDistance;

				XMVECTOR pivot = XMVectorSet(
					m_orbitPivot.x,
					m_orbitPivot.y,
					m_orbitPivot.z,
					1.0f);

				XMVECTOR newPositionVector = pivot + offset;
				XMFLOAT3 newPosition;
				XMStoreFloat3(&newPosition, newPositionVector);

				EU::Vector3 eye(newPosition.x, newPosition.y, newPosition.z);
				m_camera.lookAt(eye, m_orbitPivot);
				m_camera.setPosition(eye);
			}
			else {
				m_orbitActive = false;
			}

			// Cámara libre tipo Unreal.
			if (rightMouseDown && !altPressed) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_None);
				const float mouseSensitivity = 0.0040f;
				m_camera.yaw(io.MouseDelta.x * mouseSensitivity);
				m_camera.pitch(io.MouseDelta.y * mouseSensitivity);
			}

			// Pan con clic central.
			if (middleMouseDown) {
				const float panSensitivity = 0.020f;
				const float horizontal = -io.MouseDelta.x * panSensitivity;
				const float vertical = io.MouseDelta.y * panSensitivity;

				m_camera.strafe(horizontal);

				EU::Vector3 cameraPosition = m_camera.getPosition();
				cameraPosition.y += vertical;
				m_camera.setPosition(cameraPosition);

				// Mantener el pivote coherente después de desplazar la vista.
				EU::Vector3 right = m_camera.GetRight();
				m_orbitPivot.x += right.x * horizontal;
				m_orbitPivot.y += right.y * horizontal + vertical;
				m_orbitPivot.z += right.z * horizontal;
			}

			if (!ctrlPressed && !altPressed) {
				float movementSpeed = cameraMovementSpeed;
				if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
					movementSpeed *= 2.5f;

				const float movement = movementSpeed * deltaTime;

				if ((GetAsyncKeyState('W') & 0x8000) != 0)
					m_camera.walk(movement);
				if ((GetAsyncKeyState('S') & 0x8000) != 0)
					m_camera.walk(-movement);
				if ((GetAsyncKeyState('A') & 0x8000) != 0)
					m_camera.strafe(-movement);
				if ((GetAsyncKeyState('D') & 0x8000) != 0)
					m_camera.strafe(movement);

				EU::Vector3 cameraPosition = m_camera.getPosition();
				bool verticalChanged = false;

				if ((GetAsyncKeyState('Q') & 0x8000) != 0) {
					cameraPosition.y -= movement;
					verticalChanged = true;
				}
				if ((GetAsyncKeyState('E') & 0x8000) != 0) {
					cameraPosition.y += movement;
					verticalChanged = true;
				}
				if (verticalChanged)
					m_camera.setPosition(cameraPosition);

				const float rotationSpeed = 1.5f * deltaTime;
				if ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0)
					m_camera.yaw(-rotationSpeed);
				if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0)
					m_camera.yaw(rotationSpeed);
				if ((GetAsyncKeyState(VK_UP) & 0x8000) != 0)
					m_camera.pitch(-rotationSpeed);
				if ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0)
					m_camera.pitch(rotationSpeed);
			}
		}
		else {
			m_orbitActive = false;
		}
	}

	// --- Focus (F) y Zoom-to-fit ---
	{
		EU::TSharedPointer<Actor> selected;
		if (!m_actors.empty() && m_gui.selectedActorIndex >= 0 &&
			m_gui.selectedActorIndex < (int)m_actors.size()) {
			selected = m_actors[m_gui.selectedActorIndex];
		}
		static bool fDown = false;
		bool fNow = (GetAsyncKeyState('F') & 0x8000) != 0;
		if (m_gui.m_viewportHovered && fNow && !fDown) focusCameraOnActor(selected);
		fDown = fNow;
		if (m_gui.consumeFocusRequest()) focusCameraOnActor(selected);
		if (m_gui.consumeFitRequest())   fitCameraToScene();
	}

	// --- Picking (click izquierdo) ---
	if (m_gui.m_viewportHovered && !m_gui.m_isUsingGizmo &&
		(GetAsyncKeyState(VK_MENU) & 0x8000) == 0 &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver()) {
		pickActorFromMouse();
	}

	// --- Outline del objeto seleccionado ---
	if (!m_actors.empty() && m_gui.selectedActorIndex >= 0 &&
		m_gui.selectedActorIndex < (int)m_actors.size()) {
		EU::TSharedPointer<Actor> sel = m_actors[m_gui.selectedActorIndex];
		if (!sel.isNull()) {
			EU::TSharedPointer<Transform> t = sel->getComponent<Transform>();
			EU::Vector3 mn, mx;
			if (t && getActorAABB(sel, mn, mx)) {
				m_gui.drawSelectionOutline(m_camera, mn, mx, t->worldMatrix);
			}
		}
	}


	// Resize estable del viewport
	unsigned int desiredW = static_cast<unsigned int>(m_gui.m_viewportSize.x);
	unsigned int desiredH = static_cast<unsigned int>(m_gui.m_viewportSize.y);
	const unsigned int kMinViewportSize = 64;
	if (desiredW < kMinViewportSize) desiredW = kMinViewportSize;
	if (desiredH < kMinViewportSize) desiredH = kMinViewportSize;
	if (desiredW != m_lastRequestedViewportWidth || desiredH != m_lastRequestedViewportHeight) {
		m_lastRequestedViewportWidth = desiredW;
		m_lastRequestedViewportHeight = desiredH;
		m_viewportResizeStableFrames = 0;
	}
	else {
		m_viewportResizeStableFrames++;
	}
	const int kStableFramesRequired = 2;
	if (m_viewportResizeStableFrames >= kStableFramesRequired) {
		if (desiredW != m_editorViewportPass.getWidth() ||
			desiredH != m_editorViewportPass.getHeight()) {
			m_editorViewportResizePending = true;
			m_pendingViewportWidth = desiredW;
			m_pendingViewportHeight = desiredH;
		}
	}

	m_camera.updateViewMatrix();
	XMStoreFloat4x4(&m_constantBufferStruct.View, XMMatrixTranspose(m_camera.getView()));
	XMStoreFloat4x4(&m_constantBufferStruct.Projection, XMMatrixTranspose(m_camera.getProj()));
	m_constantBufferStruct.CameraPos = m_camera.getPosition();

	if (!m_directionalLightActor.isNull()) {
		EU::TSharedPointer<LightComponent> lightComponent = m_directionalLightActor->getComponent<LightComponent>();
		if (lightComponent) {
			lightComponent->getLightData().direction = m_constantBufferStruct.LightDir;
			lightComponent->getLightData().color = m_constantBufferStruct.LightColor;
		}
	}

	m_skybox.update(m_deviceContext, m_camera);
	m_constantBuffer.update(m_deviceContext, nullptr, 0, nullptr, &m_constantBufferStruct, 0, 0);
	m_sceneGraph.update(deltaTime, m_deviceContext);
}

void
BaseApp::render() {
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	m_deviceContext.m_drawCallCount = 0;

	m_renderScene.clear();
	m_sceneGraph.gatherRenderScene(m_renderScene, m_camera);
	m_renderScene.skybox = &m_skybox;

	m_renderPipeline.setShadowFactorDebugEnabled(m_gui.m_visualizeDeferredShadowFactor);
	m_renderPipeline.setDeferredDebugViewMode(m_gui.m_deferredDebugViewMode);
	m_renderPipeline.render(m_deviceContext, m_camera, m_renderScene, m_editorViewportPass);

	m_lastDrawCalls = m_deviceContext.m_drawCallCount;

	m_renderTargetView.render(m_deviceContext, m_depthStencilView, 1, ClearColor);
	m_viewport.render(m_deviceContext);
	m_depthStencilView.render(m_deviceContext);
	m_gui.render();
	m_swapChain.present();
}

void
BaseApp::destroy() {
	if (m_deviceContext.m_deviceContext) m_deviceContext.m_deviceContext->ClearState();
	m_sceneGraph.destroy();
	m_renderPipeline.destroy();
	m_editorViewportPass.destroy();
	m_ranaRenderMesh.destroy();
	m_ranaBodyAlbedo.destroy();
	m_ranaBodyMetallic.destroy();
	m_ranaBodyNormal.destroy();
	m_ranaBodyRoughness.destroy();
	m_ranaBodyAO.destroy();
	m_ranaHeadAlbedo.destroy();
	m_ranaHeadNormal.destroy();
	m_ranaHeadRoughness.destroy();
	m_ranaHeadAO.destroy();
	m_ranaGlassAlbedo.destroy();
	m_ranaGlassNormal.destroy();
	m_ranaGlassRoughness.destroy();
	m_ranaGlassAO.destroy();
	m_defaultRasterizer.destroy();
	m_defaultDepthStencil.destroy();
	m_defaultSampler.destroy();
	m_shaderProgram.destroy();
	m_depthStencil.destroy();
	m_depthStencilView.destroy();
	m_renderTargetView.destroy();
	m_swapChain.destroy();
	m_backBuffer.destroy();
	if (m_guiInitialized) {
		m_gui.destroy();
		m_guiInitialized = false;
	}
	delete m_ranaModel;
	m_ranaModel = nullptr;
	for (auto& lm : m_loadedModels) {
		if (lm) {
			lm->mesh.destroy();
			lm->albedo.destroy(); lm->normal.destroy(); lm->metallic.destroy();
			lm->roughness.destroy(); lm->ao.destroy();
		}
	}
	m_loadedModels.clear();
	for (auto& tex : m_thumbTextures) tex.destroy();
	m_thumbTextures.clear();

	m_deviceContext.destroy();
	m_device.destroy();
}

LRESULT
BaseApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
		return true;
	}
	switch (message) {
	case WM_CREATE: {
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCreate->lpCreateParams);
	}
				  return 0;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
				 return 0;
	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED) return 0;
		UINT newW = LOWORD(lParam);
		UINT newH = HIWORD(lParam);
		if (newW == 0 || newH == 0) return 0;
		BaseApp* app = reinterpret_cast<BaseApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (app) app->onResize(newW, newH);
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void BaseApp::onResize(unsigned int newW, unsigned int newH)
{
	if (!m_d3dReady) {
		m_window.m_width = (int)newW;
		m_window.m_height = (int)newH;
		return;
	}
	if (!m_deviceContext.m_deviceContext || !m_swapChain.m_swapChain) return;
	if (newW == 0 || newH == 0) return;

	m_window.m_width = (int)newW;
	m_window.m_height = (int)newH;

	ID3D11RenderTargetView* nullRTV = nullptr;
	m_deviceContext.m_deviceContext->OMSetRenderTargets(1, &nullRTV, nullptr);

	m_renderTargetView.destroy();
	m_depthStencilView.destroy();
	m_depthStencil.destroy();
	m_backBuffer.destroy();

	HRESULT hr = m_swapChain.resizeBuffers(newW, newH);
	if (FAILED(hr)) return;
	hr = m_swapChain.getBackBuffer(m_backBuffer);
	if (FAILED(hr)) return;
	hr = m_renderTargetView.init(m_device, m_backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);
	if (FAILED(hr)) return;
	hr = m_depthStencil.init(m_device, newW, newH, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL, 4, 0);
	if (FAILED(hr)) return;
	hr = m_depthStencilView.init(m_device, m_depthStencil, DXGI_FORMAT_D24_UNORM_S8_UINT);
	if (FAILED(hr)) return;

	m_viewport.init(m_window);
	m_camera.setLens(XM_PIDIV4, newW / (float)newH, 0.01f, 100.0f);
}

void BaseApp::handleEditorViewportResize()
{
	if (!m_editorViewportResizePending) return;

	m_deviceContext.m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	ID3D11ShaderResourceView* nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
	m_deviceContext.m_deviceContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);

	EditorViewportPass newPass;
	HRESULT hr = newPass.init(m_device, m_pendingViewportWidth, m_pendingViewportHeight);
	if (FAILED(hr)) { m_editorViewportResizePending = false; return; }

	m_editorViewportPass.swap(newPass);
	m_renderPipeline.resize(m_device, m_pendingViewportWidth, m_pendingViewportHeight);
	m_editorViewportResizePending = false;
}

void
BaseApp::captureInitialState() {
	m_initialLightDir = m_constantBufferStruct.LightDir;
	m_initialLightColor = m_constantBufferStruct.LightColor;
	m_initialCameraPos = m_camera.getPosition();
	m_initialTransforms.clear();
	for (auto& actor : m_actors) {
		InitialTransform it{};
		if (!actor.isNull()) {
			EU::TSharedPointer<Transform> t = actor->getComponent<Transform>();
			if (t) { it.position = t->getPosition(); it.rotation = t->getRotation(); it.scale = t->getScale(); }
		}
		m_initialTransforms.push_back(it);
	}
}

void
BaseApp::resetSceneToDefaults() {
	m_constantBufferStruct.LightDir = m_initialLightDir;
	m_constantBufferStruct.LightColor = m_initialLightColor;
	m_camera.setPosition(m_initialCameraPos.x, m_initialCameraPos.y, m_initialCameraPos.z);
	for (size_t i = 0; i < m_actors.size() && i < m_initialTransforms.size(); ++i) {
		if (m_actors[i].isNull()) continue;
		EU::TSharedPointer<Transform> t = m_actors[i]->getComponent<Transform>();
		if (t) {
			t->setPosition(m_initialTransforms[i].position);
			t->setRotation(m_initialTransforms[i].rotation);
			t->setScale(m_initialTransforms[i].scale);
		}
	}
	if (!m_directionalLightActor.isNull()) {
		EU::TSharedPointer<LightComponent> lc = m_directionalLightActor->getComponent<LightComponent>();
		if (lc) { lc->getLightData().direction = m_initialLightDir; lc->getLightData().color = m_initialLightColor; }
	}
	MESSAGE("BaseApp", "resetSceneToDefaults", "Escena restaurada");
}

void
BaseApp::focusCameraOnActor(const EU::TSharedPointer<Actor>& actor) {
	if (actor.isNull()) return;

	EU::TSharedPointer<Transform> transform = actor->getComponent<Transform>();
	if (!transform) return;

	EU::Vector3 localMin, localMax;
	EU::Vector3 target = transform->getPosition();
	float radius = 1.0f;

	if (getActorAABB(actor, localMin, localMax)) {
		float worldMin[3] = { 1e9f, 1e9f, 1e9f };
		float worldMax[3] = { -1e9f, -1e9f, -1e9f };

		for (int corner = 0; corner < 8; ++corner) {
			const float x = (corner & 1) ? localMax.x : localMin.x;
			const float y = (corner & 2) ? localMax.y : localMin.y;
			const float z = (corner & 4) ? localMax.z : localMin.z;

			XMVECTOR worldCorner = XMVector3TransformCoord(
				XMVectorSet(x, y, z, 1.0f),
				transform->worldMatrix);

			XMFLOAT3 value;
			XMStoreFloat3(&value, worldCorner);

			worldMin[0] = fminf(worldMin[0], value.x);
			worldMin[1] = fminf(worldMin[1], value.y);
			worldMin[2] = fminf(worldMin[2], value.z);

			worldMax[0] = fmaxf(worldMax[0], value.x);
			worldMax[1] = fmaxf(worldMax[1], value.y);
			worldMax[2] = fmaxf(worldMax[2], value.z);
		}

		target = EU::Vector3(
			(worldMin[0] + worldMax[0]) * 0.5f,
			(worldMin[1] + worldMax[1]) * 0.5f,
			(worldMin[2] + worldMax[2]) * 0.5f);

		const float dx = worldMax[0] - worldMin[0];
		const float dy = worldMax[1] - worldMin[1];
		const float dz = worldMax[2] - worldMin[2];
		radius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz);
		if (radius < 0.5f) radius = 0.5f;
	}

	const float distance =
		(radius / tanf(m_camera.getFovY() * 0.5f)) * 1.25f;

	EU::Vector3 forward = m_camera.GetForward();
	EU::Vector3 eye(
		target.x - forward.x * distance,
		target.y - forward.y * distance,
		target.z - forward.z * distance);

	m_orbitPivot = target;
	m_orbitDistance = distance;

	m_camera.lookAt(eye, target);
	m_camera.setPosition(eye);
}

void
BaseApp::fitCameraToScene() {
	float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
	float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
	int count = 0;
	for (auto& a : m_actors) {
		if (a.isNull()) continue;
		if (a->getComponent<MeshRendererComponent>().isNull()) continue;
		EU::TSharedPointer<Transform> t = a->getComponent<Transform>();
		if (!t) continue;
		EU::Vector3 p = t->getPosition();
		minX = fminf(minX, p.x); minY = fminf(minY, p.y); minZ = fminf(minZ, p.z);
		maxX = fmaxf(maxX, p.x); maxY = fmaxf(maxY, p.y); maxZ = fmaxf(maxZ, p.z);
		++count;
	}
	if (count == 0) return;
	EU::Vector3 center((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f);
	float dx = maxX - minX, dy = maxY - minY, dz = maxZ - minZ;
	float radius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz) + 3.0f;
	float dist = radius / tanf(m_camera.getFovY() * 0.5f);
	EU::Vector3 fwd = m_camera.GetForward();
	EU::Vector3 eye(center.x - fwd.x * dist, center.y - fwd.y * dist, center.z - fwd.z * dist);
	m_camera.lookAt(eye, center);
	m_camera.setPosition(eye);
}

bool
BaseApp::captureGizmoState(int index, GizmoEditState& out) {
	if (index < 0 || index >= (int)m_actors.size()) return false;
	if (m_actors[index].isNull()) return false;
	EU::TSharedPointer<Transform> t = m_actors[index]->getComponent<Transform>();
	if (!t) return false;
	out.position = t->getPosition();
	out.rotation = t->getRotation();
	out.scale = t->getScale();
	return true;
}

void
BaseApp::pickActorFromMouse() {
	const float viewportX = m_gui.m_viewportPos.x;
	const float viewportY = m_gui.m_viewportPos.y;
	const float viewportWidth = m_gui.m_viewportSize.x;
	const float viewportHeight = m_gui.m_viewportSize.y;

	if (viewportWidth < 1.0f || viewportHeight < 1.0f) return;

	ImVec2 mouse = ImGui::GetIO().MousePos;
	const float mouseX = mouse.x - viewportX;
	const float mouseY = mouse.y - viewportY;

	if (mouseX < 0.0f || mouseY < 0.0f ||
		mouseX > viewportWidth || mouseY > viewportHeight) {
		return;
	}

	const float ndcX = (2.0f * mouseX / viewportWidth) - 1.0f;
	const float ndcY = 1.0f - (2.0f * mouseY / viewportHeight);

	const XMMATRIX viewProjection =
		m_camera.getView() * m_camera.getProj();

	// XNAMath requiere una direccion valida para almacenar
	// el determinante. No se debe enviar nullptr.
	const XMVECTOR determinantCheck =
		XMMatrixDeterminant(viewProjection);

	const float determinantValue =
		XMVectorGetX(determinantCheck);

	if (!_finite(determinantValue) ||
		fabsf(determinantValue) < 0.000001f) {

		ERROR(
			"BaseApp",
			"pickActorFromMouse",
			"La matriz ViewProjection no se puede invertir.");

		return;
	}

	XMVECTOR determinant;

	const XMMATRIX inverseViewProjection =
		XMMatrixInverse(
			&determinant,
			viewProjection);

	XMVECTOR nearPoint = XMVector3TransformCoord(
		XMVectorSet(ndcX, ndcY, 0.0f, 1.0f),
		inverseViewProjection);

	XMVECTOR farPoint = XMVector3TransformCoord(
		XMVectorSet(ndcX, ndcY, 1.0f, 1.0f),
		inverseViewProjection);

	XMVECTOR rayDirectionVector =
		XMVector3Normalize(XMVectorSubtract(farPoint, nearPoint));

	XMFLOAT3 rayOrigin;
	XMFLOAT3 rayDirection;
	XMStoreFloat3(&rayOrigin, nearPoint);
	XMStoreFloat3(&rayDirection, rayDirectionVector);

	float closestDistance = FLT_MAX;
	int closestActorIndex = -1;

	for (int actorIndex = 0;
		actorIndex < static_cast<int>(m_actors.size());
		++actorIndex) {

		EU::TSharedPointer<Actor> actor = m_actors[actorIndex];
		if (actor.isNull()) continue;

		EU::TSharedPointer<MeshRendererComponent> meshRenderer =
			actor->getComponent<MeshRendererComponent>();

		EU::TSharedPointer<Transform> transform =
			actor->getComponent<Transform>();

		if (!meshRenderer || !meshRenderer->isVisible() || !transform)
			continue;

		EU::Vector3 localMin;
		EU::Vector3 localMax;
		if (!getActorAABB(actor, localMin, localMax))
			continue;

		// Broad phase: rayo contra AABB mundial.
		float worldMin[3] = { 1e9f, 1e9f, 1e9f };
		float worldMax[3] = { -1e9f, -1e9f, -1e9f };

		for (int corner = 0; corner < 8; ++corner) {
			const float x = (corner & 1) ? localMax.x : localMin.x;
			const float y = (corner & 2) ? localMax.y : localMin.y;
			const float z = (corner & 4) ? localMax.z : localMin.z;

			XMVECTOR worldCorner = XMVector3TransformCoord(
				XMVectorSet(x, y, z, 1.0f),
				transform->worldMatrix);

			XMFLOAT3 value;
			XMStoreFloat3(&value, worldCorner);

			worldMin[0] = fminf(worldMin[0], value.x);
			worldMin[1] = fminf(worldMin[1], value.y);
			worldMin[2] = fminf(worldMin[2], value.z);

			worldMax[0] = fmaxf(worldMax[0], value.x);
			worldMax[1] = fmaxf(worldMax[1], value.y);
			worldMax[2] = fmaxf(worldMax[2], value.z);
		}

		const float origin[3] = {
			rayOrigin.x, rayOrigin.y, rayOrigin.z
		};

		const float direction[3] = {
			rayDirection.x, rayDirection.y, rayDirection.z
		};

		float aabbNear = 0.0f;
		float aabbFar = FLT_MAX;
		bool aabbHit = true;

		for (int axis = 0; axis < 3; ++axis) {
			if (fabsf(direction[axis]) < 1e-8f) {
				if (origin[axis] < worldMin[axis] ||
					origin[axis] > worldMax[axis]) {
					aabbHit = false;
					break;
				}
			}
			else {
				const float inverseDirection = 1.0f / direction[axis];
				float first =
					(worldMin[axis] - origin[axis]) * inverseDirection;
				float second =
					(worldMax[axis] - origin[axis]) * inverseDirection;

				if (first > second) {
					const float temporary = first;
					first = second;
					second = temporary;
				}

				aabbNear = fmaxf(aabbNear, first);
				aabbFar = fminf(aabbFar, second);

				if (aabbNear > aabbFar) {
					aabbHit = false;
					break;
				}
			}
		}

		if (!aabbHit || aabbNear > closestDistance)
			continue;

		// Narrow phase: rayo contra triángulos CPU.
		const std::vector<MeshComponent>* cpuMeshes =
			getActorCpuMeshes(actor);

		bool preciseHit = false;
		float preciseDistance = FLT_MAX;

		if (cpuMeshes) {
			for (const MeshComponent& mesh : *cpuMeshes) {
				for (size_t index = 0;
					index + 2 < mesh.m_index.size();
					index += 3) {

					const unsigned int i0 = mesh.m_index[index + 0];
					const unsigned int i1 = mesh.m_index[index + 1];
					const unsigned int i2 = mesh.m_index[index + 2];

					if (i0 >= mesh.m_vertex.size() ||
						i1 >= mesh.m_vertex.size() ||
						i2 >= mesh.m_vertex.size()) {
						continue;
					}

					const SimpleVertex& vertex0 = mesh.m_vertex[i0];
					const SimpleVertex& vertex1 = mesh.m_vertex[i1];
					const SimpleVertex& vertex2 = mesh.m_vertex[i2];

					XMVECTOR worldVertex0 = XMVector3TransformCoord(
						XMVectorSet(
							vertex0.Position.x,
							vertex0.Position.y,
							vertex0.Position.z,
							1.0f),
						transform->worldMatrix);

					XMVECTOR worldVertex1 = XMVector3TransformCoord(
						XMVectorSet(
							vertex1.Position.x,
							vertex1.Position.y,
							vertex1.Position.z,
							1.0f),
						transform->worldMatrix);

					XMVECTOR worldVertex2 = XMVector3TransformCoord(
						XMVectorSet(
							vertex2.Position.x,
							vertex2.Position.y,
							vertex2.Position.z,
							1.0f),
						transform->worldMatrix);

					XMFLOAT3 triangle0;
					XMFLOAT3 triangle1;
					XMFLOAT3 triangle2;
					XMStoreFloat3(&triangle0, worldVertex0);
					XMStoreFloat3(&triangle1, worldVertex1);
					XMStoreFloat3(&triangle2, worldVertex2);

					float triangleDistance = 0.0f;
					if (rayTriangleIntersection(
						rayOrigin,
						rayDirection,
						triangle0,
						triangle1,
						triangle2,
						triangleDistance)) {

						preciseHit = true;
						preciseDistance =
							fminf(preciseDistance, triangleDistance);
					}
				}
			}
		}

		// Si no hay copia CPU, se conserva el fallback AABB.
		const float actorDistance =
			preciseHit ? preciseDistance : aabbNear;

		if (actorDistance < closestDistance) {
			closestDistance = actorDistance;
			closestActorIndex = actorIndex;
		}
	}

	if (closestActorIndex >= 0) {
		m_gui.selectedActorIndex = closestActorIndex;
		MESSAGE("BaseApp", "pickActorFromMouse", "Actor seleccionado desde viewport");
	}
}

void
BaseApp::addActorToScene(const EU::TSharedPointer<Actor>& actor) {
	if (actor.isNull()) return;
	m_actors.push_back(actor);
	m_sceneGraph.addEntity(actor.get());
}

void
BaseApp::removeActorFromScene(const EU::TSharedPointer<Actor>& actor) {
	if (actor.isNull()) return;
	m_sceneGraph.removeEntity(actor.get());
	for (size_t i = 0; i < m_actors.size(); ++i) {
		if (m_actors[i].get() == actor.get()) { m_actors.erase(m_actors.begin() + i); break; }
	}
	if (m_gui.selectedActorIndex >= (int)m_actors.size())
		m_gui.selectedActorIndex = (int)m_actors.size() - 1;
}

EU::TSharedPointer<Actor>
BaseApp::spawnRana(const std::string& name,
	const EU::Vector3& pos, const EU::Vector3& rot, const EU::Vector3& scale) {
	EU::TSharedPointer<Actor> a = EU::MakeShared<Actor>(m_device);
	if (a.isNull()) return a;
	a->setName(name);
	EU::TSharedPointer<Transform> t = a->getComponent<Transform>();
	if (t) t->setTransform(pos, rot, scale);
	EU::TSharedPointer<MeshRendererComponent> mr = a->getComponent<MeshRendererComponent>();
	if (!mr) { mr = EU::MakeShared<MeshRendererComponent>(); a->addComponent(mr); }
	mr->setMesh(&m_ranaRenderMesh);
	mr->setMaterialInstances({ &m_ranaBodyMaterial, &m_ranaHeadMaterial, &m_ranaGlassMaterial });
	mr->setVisible(true);
	mr->setCastShadow(true);
	return a;
}


EU::TSharedPointer<Actor>
BaseApp::spawnActorFromSource(
	const std::string& modelPath,
	const std::string& name,
	const EU::Vector3& position,
	const EU::Vector3& rotation,
	const EU::Vector3& scale) {

	EU::TSharedPointer<Actor> actor;

	if (modelPath.empty() ||
		toLowerCopy(modelPath) == toLowerCopy("Assets/Models/Rana.fbx")) {
		actor = spawnRana(name, position, rotation, scale);
		if (!actor.isNull())
			m_actorSourcePaths[actor.get()] = "Assets/Models/Rana.fbx";
	}
	else {
		actor = loadModelActor(modelPath);
		if (!actor.isNull()) {
			actor->setName(name);
			EU::TSharedPointer<Transform> transform =
				actor->getComponent<Transform>();

			if (transform)
				transform->setTransform(position, rotation, scale);

			m_actorSourcePaths[actor.get()] = modelPath;
		}
	}

	return actor;
}

const std::vector<MeshComponent>*
BaseApp::getActorCpuMeshes(
	const EU::TSharedPointer<Actor>& actor) const {

	if (actor.isNull()) return nullptr;

	EU::TSharedPointer<MeshRendererComponent> meshRenderer =
		actor->getComponent<MeshRendererComponent>();

	if (!meshRenderer || !meshRenderer->getMesh())
		return nullptr;

	Mesh* mesh = meshRenderer->getMesh();

	if (mesh == &m_ranaRenderMesh)
		return &m_ranaCpuMeshes;

	for (const auto& loadedModel : m_loadedModels) {
		if (loadedModel && &loadedModel->mesh == mesh)
			return &loadedModel->cpuMeshes;
	}

	return nullptr;
}

void
BaseApp::duplicateSelected() {
	const int index = m_gui.selectedActorIndex;

	if (index < 0 ||
		index >= static_cast<int>(m_actors.size()) ||
		m_actors[index].isNull()) {
		MESSAGE("BaseApp", "duplicateSelected", "No hay actor seleccionado");
		return;
	}

	EU::TSharedPointer<Actor> source = m_actors[index];
	EU::TSharedPointer<Transform> transform =
		source->getComponent<Transform>();

	if (!transform) return;

	EU::Vector3 position = transform->getPosition();
	position.x += 1.5f;

	std::string sourcePath;
	auto sourceIterator = m_actorSourcePaths.find(source.get());
	if (sourceIterator != m_actorSourcePaths.end())
		sourcePath = sourceIterator->second;

	EU::TSharedPointer<Actor> duplicated = spawnActorFromSource(
		sourcePath,
		source->getName() + "_copy",
		position,
		transform->getRotation(),
		transform->getScale());

	if (duplicated.isNull()) {
		ERROR("BaseApp", "duplicateSelected", "No se pudo duplicar el actor");
		return;
	}

	addActorToScene(duplicated);
	m_commands.push(std::unique_ptr<ICommand>(
		new SpawnActorCommand(this, duplicated)));

	m_gui.selectedActorIndex =
		static_cast<int>(m_actors.size()) - 1;

	MESSAGE("BaseApp", "duplicateSelected", "Actor duplicado");
}

void
BaseApp::deleteSelected() {
	int idx = m_gui.selectedActorIndex;
	if (idx < 0 || idx >= (int)m_actors.size() || m_actors[idx].isNull()) {
		MESSAGE("BaseApp", "deleteSelected", "No hay actor seleccionado");
		return;
	}
	EU::TSharedPointer<Actor> a = m_actors[idx];
	removeActorFromScene(a);
	m_commands.push(std::unique_ptr<ICommand>(new DeleteActorCommand(this, a)));
	MESSAGE("BaseApp", "deleteSelected", "Actor eliminado");
}

void
BaseApp::copySelected() {
	const int index = m_gui.selectedActorIndex;

	if (index < 0 ||
		index >= static_cast<int>(m_actors.size()) ||
		m_actors[index].isNull()) {
		MESSAGE("BaseApp", "copySelected", "No hay actor seleccionado");
		return;
	}

	EU::TSharedPointer<Actor> source = m_actors[index];
	EU::TSharedPointer<Transform> transform =
		source->getComponent<Transform>();

	if (!transform) return;

	m_clipboard.name = source->getName();
	m_clipboard.position = transform->getPosition();
	m_clipboard.rotation = transform->getRotation();
	m_clipboard.scale = transform->getScale();

	m_clipboard.modelPath.clear();
	auto sourceIterator = m_actorSourcePaths.find(source.get());
	if (sourceIterator != m_actorSourcePaths.end())
		m_clipboard.modelPath = sourceIterator->second;

	m_hasClipboard = true;
	MESSAGE("BaseApp", "copySelected", "Actor copiado al portapapeles");
}

void
BaseApp::pasteClipboard() {
	if (!m_hasClipboard) {
		MESSAGE("BaseApp", "pasteClipboard", "Portapapeles vacio");
		return;
	}

	EU::Vector3 position = m_clipboard.position;
	position.x += 1.5f;

	EU::TSharedPointer<Actor> pasted = spawnActorFromSource(
		m_clipboard.modelPath,
		m_clipboard.name + "_paste",
		position,
		m_clipboard.rotation,
		m_clipboard.scale);

	if (pasted.isNull()) {
		ERROR("BaseApp", "pasteClipboard", "No se pudo pegar el actor");
		return;
	}

	addActorToScene(pasted);
	m_commands.push(std::unique_ptr<ICommand>(
		new SpawnActorCommand(this, pasted)));

	m_gui.selectedActorIndex =
		static_cast<int>(m_actors.size()) - 1;

	MESSAGE("BaseApp", "pasteClipboard", "Actor pegado");
}

void
BaseApp::savePrefabSelected() {
	const int index = m_gui.selectedActorIndex;

	if (index < 0 ||
		index >= static_cast<int>(m_actors.size()) ||
		m_actors[index].isNull()) {
		MESSAGE("BaseApp", "savePrefabSelected", "No hay actor seleccionado");
		return;
	}

	EU::TSharedPointer<Actor> source = m_actors[index];
	EU::TSharedPointer<Transform> transform =
		source->getComponent<Transform>();

	if (!transform) return;

	CreateDirectoryA("Saved", nullptr);

	std::ofstream file("Saved/actor.prefab", std::ios::trunc);
	if (!file.is_open()) {
		ERROR("BaseApp", "savePrefabSelected", "No se pudo guardar el prefab");
		return;
	}

	std::string modelPath;
	auto sourceIterator = m_actorSourcePaths.find(source.get());
	if (sourceIterator != m_actorSourcePaths.end())
		modelPath = sourceIterator->second;

	EU::Vector3 position = transform->getPosition();
	EU::Vector3 rotation = transform->getRotation();
	EU::Vector3 scale = transform->getScale();

	bool visible = true;
	bool castShadow = true;

	EU::TSharedPointer<MeshRendererComponent> meshRenderer =
		source->getComponent<MeshRendererComponent>();

	if (meshRenderer) {
		visible = meshRenderer->isVisible();
		castShadow = meshRenderer->canCastShadow();
	}

	file << "PREFAB 2\n";
	file << "NAME " << std::quoted(source->getName()) << "\n";
	file << "MODEL " << std::quoted(modelPath) << "\n";
	file << "POSITION "
		<< position.x << " " << position.y << " " << position.z << "\n";
	file << "ROTATION "
		<< rotation.x << " " << rotation.y << " " << rotation.z << "\n";
	file << "SCALE "
		<< scale.x << " " << scale.y << " " << scale.z << "\n";
	file << "VISIBLE " << (visible ? 1 : 0) << "\n";
	file << "CAST_SHADOW " << (castShadow ? 1 : 0) << "\n";

	MESSAGE(
		"BaseApp",
		"savePrefabSelected",
		"Prefab guardado en Saved/actor.prefab");
}

void
BaseApp::loadPrefab() {
	std::ifstream file("Saved/actor.prefab");

	if (!file.is_open()) {
		ERROR("BaseApp", "loadPrefab", "No existe Saved/actor.prefab");
		return;
	}

	std::string token;
	std::string name = "Prefab";
	std::string modelPath;

	EU::Vector3 position(0.0f, 0.0f, 0.0f);
	EU::Vector3 rotation(0.0f, 0.0f, 0.0f);
	EU::Vector3 scale(1.0f, 1.0f, 1.0f);

	bool visible = true;
	bool castShadow = true;
	int version = 0;

	file >> token >> version;

	while (file >> token) {
		if (token == "NAME")
			file >> std::quoted(name);
		else if (token == "MODEL")
			file >> std::quoted(modelPath);
		else if (token == "POSITION")
			file >> position.x >> position.y >> position.z;
		else if (token == "ROTATION")
			file >> rotation.x >> rotation.y >> rotation.z;
		else if (token == "SCALE")
			file >> scale.x >> scale.y >> scale.z;
		else if (token == "VISIBLE") {
			int value = 1;
			file >> value;
			visible = value != 0;
		}
		else if (token == "CAST_SHADOW") {
			int value = 1;
			file >> value;
			castShadow = value != 0;
		}
	}

	EU::TSharedPointer<Actor> actor = spawnActorFromSource(
		modelPath,
		name,
		position,
		rotation,
		scale);

	if (actor.isNull()) {
		ERROR("BaseApp", "loadPrefab", "No se pudo crear el prefab");
		return;
	}

	EU::TSharedPointer<MeshRendererComponent> meshRenderer =
		actor->getComponent<MeshRendererComponent>();

	if (meshRenderer) {
		meshRenderer->setVisible(visible);
		meshRenderer->setCastShadow(castShadow);
	}

	addActorToScene(actor);
	m_commands.push(std::unique_ptr<ICommand>(
		new SpawnActorCommand(this, actor)));

	m_gui.selectedActorIndex =
		static_cast<int>(m_actors.size()) - 1;

	MESSAGE("BaseApp", "loadPrefab", "Prefab cargado");
}

void
BaseApp::buildTextureThumbnails() {
	m_thumbTextures.clear();
	m_thumbnails.clear();
	m_thumbTextures.reserve(64);

	std::vector<std::string> folders = listSubfolders("Assets/Textures");
	folders.push_back(""); // tambien la raiz
	int loaded = 0;
	for (const std::string& sub : folders) {
		std::string dir = sub.empty() ? "Assets/Textures" : ("Assets/Textures/" + sub);
		std::vector<std::string> files = listImageFiles(dir);
		for (const std::string& f : files) {
			if (loaded >= 48) break;
			std::string base = dir + "/" + stripExt(f);
			ExtensionType ext = extFromName(toLowerCopy(f));
			Texture tex;
			if (SUCCEEDED(tex.init(m_device, base, ext))) {
				m_thumbTextures.push_back(tex);
				AssetThumb th; th.name = f; th.srv = m_thumbTextures.back().m_textureFromImg;
				m_thumbnails.push_back(th);
				++loaded;
			}
		}
	}
	MESSAGE("BaseApp", "buildTextureThumbnails", "Thumbnails de texturas cargados");
}

void
BaseApp::loadModelTextures(LoadedModel& lm, const std::string& folder) {
	std::vector<std::string> files = listImageFiles(folder);
	for (const std::string& f : files) {
		std::string lower = toLowerCopy(f);
		std::string baseNoExt = toLowerCopy(stripExt(f));
		std::string path = folder + "/" + stripExt(f);
		ExtensionType ext = extFromName(lower);

		int slot = -1; // 0 albedo, 1 normal, 2 metallic, 3 roughness, 4 ao
		if (containsStr(lower, "basecolor") || containsStr(lower, "albedo") || containsStr(lower, "diffuse")) slot = 0;
		else if (containsStr(lower, "normal")) slot = 1;
		else if (containsStr(lower, "roughness")) slot = 3;
		else if (containsStr(lower, "metallic") || containsStr(lower, "metalness")) slot = 2;
		else if (containsStr(lower, "occlusion")) slot = 4;
		else if (endsWith(baseNoExt, "_bc") || endsWith(baseNoExt, "_d") || endsWith(baseNoExt, "_alb")) slot = 0;
		else if (endsWith(baseNoExt, "_n") || endsWith(baseNoExt, "_nrm")) slot = 1;
		else if (endsWith(baseNoExt, "_r") || endsWith(baseNoExt, "_rgh")) slot = 3;
		else if (endsWith(baseNoExt, "_m") || endsWith(baseNoExt, "_met")) slot = 2;
		else if (endsWith(baseNoExt, "_ao") || endsWith(baseNoExt, "_o")) slot = 4;

		HRESULT hr;
		switch (slot) {
		case 0: hr = lm.albedo.init(m_device, path, ext);    if (SUCCEEDED(hr)) lm.materialInstance.setAlbedo(&lm.albedo); break;
		case 1: hr = lm.normal.init(m_device, path, ext);    if (SUCCEEDED(hr)) lm.materialInstance.setNormal(&lm.normal); break;
		case 2: hr = lm.metallic.init(m_device, path, ext);  if (SUCCEEDED(hr)) lm.materialInstance.setMetallic(&lm.metallic); break;
		case 3: hr = lm.roughness.init(m_device, path, ext); if (SUCCEEDED(hr)) lm.materialInstance.setRoughness(&lm.roughness); break;
		case 4: hr = lm.ao.init(m_device, path, ext);        if (SUCCEEDED(hr)) lm.materialInstance.setAO(&lm.ao); break;
		default: break;
		}
	}
	if (!lm.albedo.m_textureFromImg) {
		lm.materialInstance.setAlbedo(&m_ranaBodyAlbedo); // fallback para no quedar negro
		MESSAGE("BaseApp", "loadModelTextures", "Sin albedo en la carpeta; usando textura fallback");
	}
}

EU::TSharedPointer<Actor>
BaseApp::loadModelActor(const std::string& modelPath) {
	std::string lower = toLowerCopy(modelPath);
	ModelType type = FBX;
	if (endsWith(lower, ".obj")) type = OBJ;

	Model3D model(modelPath, type);
	const std::vector<MeshComponent>& meshes = model.GetMeshes();
	if (meshes.empty()) {
		ERROR("BaseApp", "loadModelActor", "El modelo no tiene mallas");
		return EU::TSharedPointer<Actor>();
	}

	std::unique_ptr<LoadedModel> lm(new LoadedModel());
	lm->cpuMeshes = meshes;
	lm->sourcePath = modelPath;

	HRESULT hr;
	for (const MeshComponent& mc : meshes) {
		Submesh sm{};
		hr = sm.vertexBuffer.init(m_device, mc, D3D11_BIND_VERTEX_BUFFER);
		if (FAILED(hr)) { ERROR("BaseApp", "loadModelActor", "Fallo vertex buffer"); return EU::TSharedPointer<Actor>(); }
		hr = sm.indexBuffer.init(m_device, mc, D3D11_BIND_INDEX_BUFFER);
		if (FAILED(hr)) { ERROR("BaseApp", "loadModelActor", "Fallo index buffer"); return EU::TSharedPointer<Actor>(); }
		sm.indexCount = mc.m_numIndex;
		sm.startIndex = 0;
		sm.materialSlot = 0;
		lm->mesh.getSubmeshes().push_back(std::move(sm));
	}
	lm->localMin = EU::Vector3(1e9f, 1e9f, 1e9f);
	lm->localMax = EU::Vector3(-1e9f, -1e9f, -1e9f);
	for (const MeshComponent& mc : meshes) {
		for (const SimpleVertex& v : mc.m_vertex) {
			lm->localMin.x = fminf(lm->localMin.x, v.Position.x);
			lm->localMin.y = fminf(lm->localMin.y, v.Position.y);
			lm->localMin.z = fminf(lm->localMin.z, v.Position.z);
			lm->localMax.x = fmaxf(lm->localMax.x, v.Position.x);
			lm->localMax.y = fmaxf(lm->localMax.y, v.Position.y);
			lm->localMax.z = fmaxf(lm->localMax.z, v.Position.z);
		}
	}



	lm->material.setShader(&m_shaderProgram);
	lm->material.setRasterizerState(&m_defaultRasterizer);
	lm->material.setDepthStencilState(&m_defaultDepthStencil);
	lm->material.setSamplerState(&m_defaultSampler);
	lm->material.setDomain(MaterialDomain::Opaque);
	lm->material.setBlendMode(BlendMode::Opaque);

	lm->materialInstance.setMaterial(&lm->material);
	lm->materialInstance.getParams().baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lm->materialInstance.getParams().metallic = 1.0f;
	lm->materialInstance.getParams().roughness = 1.0f;
	lm->materialInstance.getParams().ao = 1.0f;
	lm->materialInstance.getParams().normalScale = 1.0f;
	lm->materialInstance.getParams().emissiveStrength = 1.0f;
	lm->materialInstance.getParams().alphaCutoff = 0.5f;

	std::string modelName = fileBaseName(modelPath);
	loadModelTextures(*lm, "Assets/Textures/" + modelName);

	EU::TSharedPointer<Actor> a = EU::MakeShared<Actor>(m_device);
	if (a.isNull()) return a;
	a->setName(modelName);
	EU::TSharedPointer<Transform> t = a->getComponent<Transform>();
	if (t) t->setTransform(EU::Vector3(0.0f, 2.92f, 5.60f), EU::Vector3(0.0f, 0.0f, 0.0f), EU::Vector3(1.0f, 1.0f, 1.0f));
	EU::TSharedPointer<MeshRendererComponent> mr = a->getComponent<MeshRendererComponent>();
	if (!mr) { mr = EU::MakeShared<MeshRendererComponent>(); a->addComponent(mr); }
	mr->setMesh(&lm->mesh);
	mr->setMaterialInstance(&lm->materialInstance);
	mr->setVisible(true);
	mr->setCastShadow(true);

	m_actorSourcePaths[a.get()] = modelPath;
	m_loadedModels.push_back(std::move(lm));
	return a;
}

bool
BaseApp::getActorAABB(const EU::TSharedPointer<Actor>& actor, EU::Vector3& outMin, EU::Vector3& outMax) {
	if (actor.isNull()) return false;
	EU::TSharedPointer<MeshRendererComponent> mr = actor->getComponent<MeshRendererComponent>();
	if (!mr) return false;
	Mesh* mesh = mr->getMesh();
	if (!mesh) return false;

	if (mesh == &m_ranaRenderMesh) { outMin = m_ranaModelLocalMin; outMax = m_ranaModelLocalMax; return true; }
	for (auto& lm : m_loadedModels) {
		if (lm && &lm->mesh == mesh) { outMin = lm->localMin; outMax = lm->localMax; return true; }
	}
	return false;
}
