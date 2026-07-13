#include "GUI/GUI.h"
#include "Viewport.h"
#include "Window.h"
#include "Device.h"
#include "DeviceContext.h"
#include "MeshComponent.h"
#include "ECS/Actor.h"
#include "EngineUtilities/Utilities/Camera.h"
#include "ECS/MeshRendererComponent.h"
#include <string>
#include <vector>


static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);

static const ImVec4 kAccent = ImVec4(0.55f, 0.35f, 0.90f, 1.0f);
static const ImVec4 kAccentHi = ImVec4(0.70f, 0.50f, 1.00f, 1.0f);

void GUI::awake() {}

void GUI::init(Window& window, Device& device, DeviceContext& deviceContext) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	appleLiquidStyle(1.0f, kAccent);

	ImGui_ImplWin32_Init(window.m_hWnd);
	ImGui_ImplDX11_Init(device.m_device, deviceContext.m_deviceContext);

	toolTipData();
	selectedActorIndex = 0;
}

void GUI::appleLiquidStyle(float opacity, ImVec4 accent) {
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	style.WindowRounding = 8.0f;  style.ChildRounding = 6.0f;  style.FrameRounding = 5.0f;
	style.PopupRounding = 6.0f;   style.TabRounding = 6.0f;    style.GrabRounding = 5.0f;
	style.ScrollbarRounding = 12.0f; style.WindowBorderSize = 1.0f; style.FrameBorderSize = 0.0f;
	style.WindowPadding = ImVec2(12.0f, 12.0f); style.FramePadding = ImVec2(10.0f, 6.0f);
	style.ItemSpacing = ImVec2(10.0f, 8.0f);    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
	style.IndentSpacing = 18.0f; style.ScrollbarSize = 13.0f; style.GrabMinSize = 10.0f;
	style.WindowTitleAlign = ImVec2(0.02f, 0.5f); style.WindowMenuButtonPosition = ImGuiDir_None;

	const ImVec4 bg0 = ImVec4(0.090f, 0.075f, 0.130f, opacity);
	const ImVec4 bg1 = ImVec4(0.140f, 0.120f, 0.195f, opacity);
	const ImVec4 bg2 = ImVec4(0.200f, 0.165f, 0.290f, opacity);
	const ImVec4 bg3 = ImVec4(0.270f, 0.220f, 0.380f, opacity);
	const ImVec4 txt = ImVec4(0.92f, 0.90f, 0.97f, 1.0f);
	const ImVec4 txtD = ImVec4(0.56f, 0.52f, 0.64f, 1.0f);

	colors[ImGuiCol_Text] = txt;                 colors[ImGuiCol_TextDisabled] = txtD;
	colors[ImGuiCol_WindowBg] = bg0;             colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0.12f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.09f, 0.16f, 0.98f);
	colors[ImGuiCol_Border] = ImVec4(0.34f, 0.27f, 0.48f, 0.50f);
	colors[ImGuiCol_FrameBg] = bg1;              colors[ImGuiCol_FrameBgHovered] = bg2;  colors[ImGuiCol_FrameBgActive] = bg3;
	colors[ImGuiCol_TitleBg] = ImVec4(0.075f, 0.062f, 0.110f, 1.0f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.11f, 0.21f, 1.0f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.085f, 0.070f, 0.120f, 1.0f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0.22f);
	colors[ImGuiCol_ScrollbarGrab] = bg2; colors[ImGuiCol_ScrollbarGrabHovered] = bg3; colors[ImGuiCol_ScrollbarGrabActive] = accent;
	colors[ImGuiCol_CheckMark] = kAccentHi;
	colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.34f, 0.82f, 1.0f); colors[ImGuiCol_SliderGrabActive] = kAccentHi;
	colors[ImGuiCol_Button] = bg1; colors[ImGuiCol_ButtonHovered] = bg2; colors[ImGuiCol_ButtonActive] = accent;
	colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.28f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.48f);
	colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.68f);
	colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.24f, 0.42f, 0.55f);
	colors[ImGuiCol_SeparatorHovered] = accent; colors[ImGuiCol_SeparatorActive] = kAccentHi;
	colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.55f);
	colors[ImGuiCol_ResizeGripActive] = kAccentHi;
	colors[ImGuiCol_Tab] = bg1; colors[ImGuiCol_TabHovered] = ImVec4(accent.x, accent.y, accent.z, 0.65f);
	colors[ImGuiCol_TabActive] = ImVec4(0.32f, 0.24f, 0.46f, 1.0f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.085f, 0.15f, 1.0f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.13f, 0.23f, 1.0f);
	colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.55f);
	colors[ImGuiCol_DockingEmptyBg] = bg0;
	colors[ImGuiCol_PlotLines] = kAccentHi; colors[ImGuiCol_PlotHistogram] = accent;
	colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
	colors[ImGuiCol_NavHighlight] = kAccentHi;

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		colors[ImGuiCol_WindowBg].w = 1.0f;
	}
}

void GUI::update(Viewport& viewport, Window& window) {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);

	drawStudioTopRibbon();
	drawEditorDockspace();
	closeApp();
	drawGizmoToolbar();
}

void GUI::render() {
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void GUI::destroy() {
	if (ImGui::GetCurrentContext() == nullptr) return;
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void GUI::vec3Control(const std::string& label, float* values, float resetValue, float columnWidth) {
	ImGuiIO& io = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];

	ImGui::PushID(label.c_str());
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text("%s", label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 2.0f, 0.0f });
	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.70f, 0.22f, 0.24f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.82f, 0.30f, 0.32f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.60f, 0.16f, 0.18f, 1.0f });
	ImGui::PushFont(boldFont);
	if (ImGui::Button("X", buttonSize)) values[0] = resetValue;
	ImGui::PopFont(); ImGui::PopStyleColor(3); ImGui::SameLine();
	ImGui::DragFloat("##X", &values[0], 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth(); ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.28f, 0.58f, 0.30f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.36f, 0.68f, 0.38f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.20f, 0.50f, 0.22f, 1.0f });
	ImGui::PushFont(boldFont);
	if (ImGui::Button("Y", buttonSize)) values[1] = resetValue;
	ImGui::PopFont(); ImGui::PopStyleColor(3); ImGui::SameLine();
	ImGui::DragFloat("##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth(); ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.20f, 0.42f, 0.80f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.28f, 0.52f, 0.90f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.14f, 0.34f, 0.70f, 1.0f });
	ImGui::PushFont(boldFont);
	if (ImGui::Button("Z", buttonSize)) values[2] = resetValue;
	ImGui::PopFont(); ImGui::PopStyleColor(3); ImGui::SameLine();
	ImGui::DragFloat("##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();
	ImGui::Columns(1);
	ImGui::PopID();
}

void GUI::toolTipData() {}
void GUI::ToolBar() {}

void GUI::closeApp() {
	if (show_exit_popup) { ImGui::OpenPopup("Exit?"); show_exit_popup = false; }
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Exit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Estas a punto de salir de MinerEngine.");
		ImGui::Text("Estas seguro?");
		ImGui::Spacing(); ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { exit(0); ImGui::CloseCurrentPopup(); }
		ImGui::SetItemDefaultFocus(); ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}

void GUI::inspectorGeneral(EU::TSharedPointer<Actor> actor) {
	ImGui::Begin("Inspector");
	if (!actor) { ImGui::TextDisabled("No actor selected"); ImGui::End(); return; }

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.10f, 0.17f, 1.0f));
	ImGui::BeginChild("HeaderRegion", ImVec2(0, 95), true);
	bool isStatic = false;
	ImGui::Checkbox("##Static", &isStatic); ImGui::SameLine();
	char objectName[128];
	strcpy_s(objectName, sizeof(objectName), actor->getName().c_str());
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
	if (ImGui::InputText("##ObjectName", objectName, IM_ARRAYSIZE(objectName))) actor->setName(std::string(objectName));
	ImGui::SameLine(); ImGui::Button("Icon", ImVec2(30, 0)); ImGui::Spacing();
	const char* tags[] = { "Untagged", "Player", "Enemy", "Environment" };
	static int currentTag = 0;
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.45f);
	ImGui::Combo("Tag", &currentTag, tags, IM_ARRAYSIZE(tags)); ImGui::SameLine();
	const char* layers[] = { "Default", "TransparentFX", "Ignore Raycast", "Water", "UI" };
	static int currentLayer = 0;
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::Combo("Layer", &currentLayer, layers, IM_ARRAYSIZE(layers));
	ImGui::EndChild();
	ImGui::PopStyleColor(); ImGui::Spacing();

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) inspectorContainer(actor);
	ImGui::Spacing();
	if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Indent(10.0f);

		auto mr = actor->getComponent<MeshRendererComponent>();
		if (mr) {
			bool vis = mr->isVisible();
			if (ImGui::Checkbox("Visible", &vis)) mr->setVisible(vis);
		}

		bool castShadow = actor->canCastShadow();
		if (ImGui::Checkbox("Cast Shadows", &castShadow)) actor->setCastShadow(castShadow);

		ImGui::TextDisabled("Configuraciones de luz y material...");
		ImGui::Unindent(10.0f);
	}
	ImGui::End();
}


void GUI::inspectorContainer(EU::TSharedPointer<Actor> actor) {
	if (!actor) return;
	auto transform = actor->getComponent<Transform>();
	if (!transform) return;
	vec3Control("Position", const_cast<float*>(transform->getPosition().data()), 0.0f, 75.0f);
	vec3Control("Rotation", const_cast<float*>(transform->getRotation().data()), 0.0f, 75.0f);
	vec3Control("Scale", const_cast<float*>(transform->getScale().data()), 1.0f, 75.0f);
}

void GUI::outliner(const std::vector<EU::TSharedPointer<Actor>>& actors) {
	ImGui::Begin("Hierarchy");

	if (ImGui::Button("Show All")) {
		for (const auto& a : actors) {
			if (a.isNull()) continue;
			auto mr = a->getComponent<MeshRendererComponent>();
			if (mr) mr->setVisible(true);
		}
	}
	ImGui::SameLine();
	static ImGuiTextFilter filter;
	filter.Draw("Search", ImGui::GetContentRegionAvail().x - 8.0f);
	ImGui::Separator();

	for (int i = 0; i < (int)actors.size(); ++i) {
		const auto& actor = actors[i];
		if (!actor) continue;
		std::string actorName = actor->getName();
		if (!filter.PassFilter(actorName.c_str())) continue;

		ImGui::PushID(i);

		auto mr = actor->getComponent<MeshRendererComponent>();
		if (mr) {
			bool vis = mr->isVisible();
			if (ImGui::Checkbox("##vis", &vis)) mr->setVisible(vis);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(vis ? "Visible (click para ocultar)" : "Oculto (click para mostrar)");
		}
		else {
			ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));
		}
		ImGui::SameLine();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (selectedActorIndex == i) flags |= ImGuiTreeNodeFlags_Selected;

		bool hidden = (mr && !mr->isVisible());
		if (hidden) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.48f, 0.56f, 1.0f));
		bool nodeOpen = ImGui::TreeNodeEx("##node", flags, "%s", actorName.c_str());
		if (hidden) ImGui::PopStyleColor();

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) selectedActorIndex = i;

		// ===== PUNTO (b): menu de clic derecho =====
		if (ImGui::BeginPopupContextItem()) {
			selectedActorIndex = i;
			ImGui::TextDisabled("Actor Options");
			ImGui::Separator();

			if (ImGui::MenuItem("Duplicate", "Ctrl+D")) m_duplicateRequested = true;
			if (ImGui::MenuItem("Copy", "Ctrl+C")) m_copyRequested = true;
			if (ImGui::MenuItem("Paste", "Ctrl+V")) m_pasteRequested = true;

			ImGui::Separator();
			if (ImGui::MenuItem("Isolate (solo)")) {
				for (const auto& a2 : actors) {
					if (a2.isNull()) continue;
					auto mr2 = a2->getComponent<MeshRendererComponent>();
					if (mr2) mr2->setVisible(a2.get() == actor.get());
				}
			}
			if (ImGui::MenuItem("Show All")) {
				for (const auto& a2 : actors) {
					if (a2.isNull()) continue;
					auto mr2 = a2->getComponent<MeshRendererComponent>();
					if (mr2) mr2->setVisible(true);
				}
			}
			if (mr) {
				bool vis = mr->isVisible();
				if (ImGui::MenuItem(vis ? "Hide" : "Show")) mr->setVisible(!vis);
			}

			ImGui::Separator();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
			if (ImGui::MenuItem("Delete", "Del")) m_deleteRequested = true;
			ImGui::PopStyleColor();

			ImGui::EndPopup();
		}
		// ===========================================

		if (nodeOpen) {
			auto transform = actor->getComponent<Transform>();
			if (transform)
				ImGui::TextDisabled("   Pos: %.1f, %.1f, %.1f",
					transform->getPosition().x, transform->getPosition().y, transform->getPosition().z);
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	ImGui::End();
}

void GUI::editTransform(Camera& cam, Window& window, EU::TSharedPointer<Actor> actor) {
	if (!actor) return;
	static ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::WORLD;
	auto transform = actor->getComponent<Transform>();
	if (!transform) return;

	float rectX = m_viewportPos.x, rectY = m_viewportPos.y;
	float rectW = m_viewportSize.x, rectH = m_viewportSize.y;
	if (rectW < 64.0f || rectH < 64.0f) { m_isUsingGizmo = false; return; }

	float* pos = const_cast<float*>(transform->getPosition().data());
	float* rot = const_cast<float*>(transform->getRotation().data());
	float* sca = const_cast<float*>(transform->getScale().data());

	float mArr[16];
	ImGuizmo::RecomposeMatrixFromComponents(pos, rot, sca, mArr);
	float vArr[16], pArr[16];
	ToFloatArray(cam.getView(), vArr);
	ToFloatArray(cam.getProj(), pArr);

	ImGuizmo::SetOrthographic(false);
	if (m_viewportDrawList) ImGuizmo::SetDrawlist(m_viewportDrawList);
	else ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
	ImGuizmo::SetID(0);
	ImGuizmo::SetGizmoSizeClipSpace(0.15f);
	ImGuizmo::AllowAxisFlip(false);
	ImGuizmo::SetRect(rectX, rectY, rectW, rectH);

	float snapValue = m_snapScale;
	if (mCurrentGizmoOperation == ImGuizmo::ROTATE)         snapValue = m_snapRotate;
	else if (mCurrentGizmoOperation == ImGuizmo::TRANSLATE) snapValue = m_snapTranslate;
	float snap[3] = { snapValue, snapValue, snapValue };
	bool useSnap = m_snapEnabled || ImGui::GetIO().KeyCtrl;
	bool canManipulate = m_viewportHovered || m_viewportActive || m_isUsingGizmo;

	if (canManipulate)
		ImGuizmo::Manipulate(vArr, pArr, mCurrentGizmoOperation, mCurrentGizmoMode, mArr, nullptr, useSnap ? snap : nullptr);

	m_isUsingGizmo = ImGuizmo::IsUsing();
	if (m_isUsingGizmo) {
		float newPos[3], newRot[3], newSca[3];
		ImGuizmo::DecomposeMatrixToComponents(mArr, newPos, newRot, newSca);
		transform->setPosition(EU::Vector3(newPos[0], newPos[1], newPos[2]));
		transform->setRotation(EU::Vector3(newRot[0], newRot[1], newRot[2]));
		transform->setScale(EU::Vector3(newSca[0], newSca[1], newSca[2]));
	}
}

void GUI::drawGizmoToolbar() {
	ImGui::SetNextWindowBgAlpha(0.85f);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	if (ImGui::Begin("GizmoToolBar", nullptr, window_flags)) {
		auto buttonMode = [&](const char* label, ImGuizmo::OPERATION op, const char* shortcut) {
			bool isActive = (mCurrentGizmoOperation == op);
			if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
			if (ImGui::Button(label)) mCurrentGizmoOperation = op;
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s (%s)", label, shortcut);
			if (isActive) ImGui::PopStyleColor();
			ImGui::SameLine();
			};
		buttonMode("T", ImGuizmo::TRANSLATE, "W");
		buttonMode("R", ImGuizmo::ROTATE, "E");
		buttonMode("S", ImGuizmo::SCALE, "R");

		static ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::WORLD;
		if (ImGui::Button(mCurrentGizmoMode == ImGuizmo::WORLD ? "Global" : "Local"))
			mCurrentGizmoMode = (mCurrentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
		ImGui::SameLine(); ImGui::TextDisabled("|"); ImGui::SameLine();
		ImGui::Checkbox("Grid", &m_showGrid); ImGui::SameLine();
		ImGui::Checkbox("Snap", &m_snapEnabled); ImGui::SameLine();
		if (ImGui::Button("Focus")) m_focusRequested = true;
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Centrar camara en el objeto (tecla F)");
		ImGui::SameLine();
		if (ImGui::Button("Fit")) m_fitRequested = true;
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Encuadrar toda la escena");
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void GUI::drawStudioTopRibbon() {
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const float menuBarHeight = 24.0f;
	const float ribbonHeight = 72.0f;

	ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, menuBarHeight), ImGuiCond_Always);
	ImGuiWindowFlags menuFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.085f, 0.070f, 0.120f, 1.0f));
	if (ImGui::Begin("##StudioMenuBar", nullptr, menuFlags)) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				ImGui::MenuItem("New Scene"); ImGui::MenuItem("Open Scene..."); ImGui::MenuItem("Save");
				ImGui::Separator();
				if (ImGui::MenuItem("Exit MinerEngine")) show_exit_popup = true;
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit")) {
				if (ImGui::MenuItem("Undo", "Ctrl+Z")) m_undoRequested = true;
				if (ImGui::MenuItem("Redo", "Ctrl+Y")) m_redoRequested = true;
				ImGui::Separator();
				if (ImGui::MenuItem("Copy", "Ctrl+C")) m_copyRequested = true;
				if (ImGui::MenuItem("Paste", "Ctrl+V")) m_pasteRequested = true;
				if (ImGui::MenuItem("Duplicate", "Ctrl+D")) m_duplicateRequested = true;
				if (ImGui::MenuItem("Delete", "Del")) m_deleteRequested = true;
				ImGui::Separator();
				if (ImGui::MenuItem("Save Prefab")) m_savePrefabRequested = true;
				if (ImGui::MenuItem("Load Prefab")) m_loadPrefabRequested = true;
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor(); ImGui::PopStyleVar(2);

	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ribbonHeight), ImGuiCond_Always);
	ImGuiWindowFlags ribbonFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.125f, 0.105f, 0.180f, 1.0f));
	if (ImGui::Begin("##StudioRibbon", nullptr, ribbonFlags)) {
		auto ribbonButton = [&](const char* id, const char* topText, const char* bottomText, ImVec2 size, bool active = false) -> bool {
			if (active) ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
			bool pressed = ImGui::Button(id, size);
			ImVec2 mn = ImGui::GetItemRectMin(); ImVec2 mx = ImGui::GetItemRectMax();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			ImVec2 ts = ImGui::CalcTextSize(topText); ImVec2 bs = ImGui::CalcTextSize(bottomText);
			float cx = (mn.x + mx.x) * 0.5f;
			dl->AddText(ImVec2(cx - ts.x * 0.5f, mn.y + 10.0f), ImGui::GetColorU32(ImGuiCol_Text), topText);
			dl->AddText(ImVec2(cx - bs.x * 0.5f, mn.y + 34.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), bottomText);
			if (active) ImGui::PopStyleColor();
			return pressed;
			};
		auto separatorGroup = [&]() {
			ImGui::SameLine(); ImGui::Dummy(ImVec2(6.0f, 1.0f)); ImGui::SameLine();
			ImVec2 p = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y), ImVec2(p.x, p.y + 48.0f), IM_COL32(140, 90, 230, 110), 1.0f);
			ImGui::Dummy(ImVec2(8.0f, 48.0f)); ImGui::SameLine();
			};
		const ImVec2 btnSize(72.0f, 52.0f);
		ribbonButton("##Select", "Select", "Cursor", btnSize, false); ImGui::SameLine();
		if (ribbonButton("##Move", "Move", "W", btnSize, mCurrentGizmoOperation == ImGuizmo::TRANSLATE)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE; ImGui::SameLine();
		if (ribbonButton("##Rotate", "Rotate", "E", btnSize, mCurrentGizmoOperation == ImGuizmo::ROTATE)) mCurrentGizmoOperation = ImGuizmo::ROTATE; ImGui::SameLine();
		if (ribbonButton("##Scale", "Scale", "R", btnSize, mCurrentGizmoOperation == ImGuizmo::SCALE)) mCurrentGizmoOperation = ImGuizmo::SCALE;
		separatorGroup();
		ribbonButton("##Part", "3D Object", "Mesh", btnSize, false); ImGui::SameLine();
		ribbonButton("##Light", "Light", "Point", btnSize, false); ImGui::SameLine();
		ribbonButton("##Material", "Material", "Editor", btnSize, false);
		separatorGroup();
		ribbonButton("##Play", "Play", "Game", btnSize, false);
	}
	ImGui::End();
	ImGui::PopStyleColor(1); ImGui::PopStyleVar(3);
}

void GUI::drawViewportPanel(ID3D11ShaderResourceView* viewportSRV) {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (ImGui::Begin("Viewport", nullptr, flags)) {
		m_viewportDrawList = ImGui::GetWindowDrawList();
		ImVec2 panelMin = ImGui::GetCursorScreenPos();
		ImVec2 panelSize = ImGui::GetContentRegionAvail();
		if (panelSize.x < 1.0f) panelSize.x = 1.0f;
		if (panelSize.y < 1.0f) panelSize.y = 1.0f;
		m_viewportPos = panelMin; m_viewportSize = panelSize;
		if (viewportSRV) ImGui::Image((ImTextureID)viewportSRV, panelSize);
		else {
			ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
			m_viewportDrawList->AddRectFilled(panelMin, panelMax, IM_COL32(24, 20, 34, 255));
			m_viewportDrawList->AddText(ImVec2(panelMin.x + 12.0f, panelMin.y + 12.0f), IM_COL32(220, 215, 240, 255), "Viewport no renderizado");
		}
		m_viewportHovered = ImGui::IsItemHovered();
		m_viewportActive = ImGui::IsItemActive();
		m_viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void GUI::drawViewportGrid(Camera& cam) {
	if (!m_showGrid) return;
	if (m_viewportSize.x < 16.0f || m_viewportSize.y < 16.0f) return;
	float view[16], proj[16], identity[16];
	ToFloatArray(cam.getView(), view);
	ToFloatArray(cam.getProj(), proj);
	ToFloatArray(XMMatrixIdentity(), identity);
	if (m_viewportDrawList) ImGuizmo::SetDrawlist(m_viewportDrawList);
	ImGuizmo::SetRect(m_viewportPos.x, m_viewportPos.y, m_viewportSize.x, m_viewportSize.y);
	ImGuizmo::DrawGrid(view, proj, identity, m_gridSize);
}

void GUI::drawEditorDockspace() {
	ImGuiViewport* mainViewport = ImGui::GetMainViewport();
	const float topOffset = 96.0f;
	ImVec2 dockPos = ImVec2(mainViewport->Pos.x, mainViewport->Pos.y + topOffset);
	ImVec2 dockSize = ImVec2(mainViewport->Size.x, mainViewport->Size.y - topOffset);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings;
	ImGui::SetNextWindowPos(dockPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(dockSize, ImGuiCond_Always);
	ImGui::SetNextWindowViewport(mainViewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("##MainEditorDockspace", nullptr, window_flags);
	ImGuiID dockspace_id = ImGui::GetID("##EditorDockspace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	if (!m_dockLayoutInitialized) {
		m_dockLayoutInitialized = true;
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, dockSize);
		ImGuiID dockMain = dockspace_id;
		ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.19f, nullptr, &dockMain);
		ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.26f, nullptr, &dockMain);
		ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.28f, nullptr, &dockMain);
		ImGuiID dockLeftBottom = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.45f, nullptr, &dockLeft);
		ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
		ImGui::DockBuilderDockWindow("Lighting", dockLeftBottom);
		ImGui::DockBuilderDockWindow("Inspector", dockRight);
		ImGui::DockBuilderDockWindow("G-Buffer", dockRight);
		ImGui::DockBuilderDockWindow("Console", dockBottom);
		ImGui::DockBuilderDockWindow("Render", dockBottom);
		ImGui::DockBuilderDockWindow("Performance", dockBottom);
		ImGui::DockBuilderDockWindow("Viewport", dockMain);
		ImGui::DockBuilderDockWindow("Content", dockBottom);
        ImGui::DockBuilderFinish(dockspace_id);
	}
	ImGui::End();
	ImGui::PopStyleVar(3);
}

void GUI::drawGBufferDebugPanel(ID3D11ShaderResourceView* albedoMetallicSRV,
	ID3D11ShaderResourceView* normalRoughnessSRV,
	ID3D11ShaderResourceView* worldAoSRV,
	ID3D11ShaderResourceView* emissiveAlphaSRV) {
	ImGui::Begin("G-Buffer");
	const char* modes[] = { "Final Lit", "Shadow Factor", "Albedo", "World Normal", "World Position", "Metal / Rough / AO", "Emissive" };
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.55f, 1.0f, 1.0f));
	ImGui::TextUnformatted("Modo de visualizacion");
	ImGui::PopStyleColor();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::Combo("##DeferredDebugMode", &m_deferredDebugViewMode, modes, IM_ARRAYSIZE(modes));
	ImGui::Checkbox("Visualizar factor de sombra", &m_visualizeDeferredShadowFactor);
	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

	auto placeholder = [&](ImVec2 size, const char* msg) {
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(25, 22, 36, 255), 4.0f);
		dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(140, 90, 230, 120), 4.0f);
		dl->AddText(ImVec2(p.x + 8.0f, p.y + 8.0f), IM_COL32(205, 200, 225, 255), msg);
		ImGui::Dummy(size);
		};
	float fullW = ImGui::GetContentRegionAvail().x;
	float cellW = (fullW - 8.0f) * 0.5f;
	ImVec2 cell(cellW, cellW * 0.5625f);
	auto target = [&](const char* label, ID3D11ShaderResourceView* srv) {
		ImGui::BeginGroup();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.72f, 0.98f, 1.0f));
		ImGui::TextUnformatted(label);
		ImGui::PopStyleColor();
		if (srv) {
			ImGui::Image((ImTextureID)srv, cell);
			if (ImGui::IsItemClicked()) { m_previewSRV = srv; m_previewLabel = label; m_showPreview = true; }
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click para ampliar: %s", label);
		}
		else placeholder(cell, "N/A");
		ImGui::EndGroup();
		};
	target("Albedo + Metallic", albedoMetallicSRV);   ImGui::SameLine();
	target("Normal + Roughness", normalRoughnessSRV);
	ImGui::Spacing();
	target("World Pos + AO", worldAoSRV);             ImGui::SameLine();
	target("Emissive + Alpha", emissiveAlphaSRV);
	ImGui::End();
}

void GUI::drawRenderDebugPanel(ID3D11ShaderResourceView* preShadowSRV,
	ID3D11ShaderResourceView* viewportSRV,
	ID3D11ShaderResourceView* shadowMapSRV) {
	ImGui::Begin("Render");
	auto placeholder = [&](ImVec2 size, const char* msg) {
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(25, 22, 36, 255), 4.0f);
		dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(140, 90, 230, 120), 4.0f);
		dl->AddText(ImVec2(p.x + 10.0f, p.y + 10.0f), IM_COL32(205, 200, 225, 255), msg);
		ImGui::Dummy(size);
		};
	auto section = [&](const char* label, ID3D11ShaderResourceView* srv, ImVec2 size) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.55f, 1.0f, 1.0f));
		ImGui::TextUnformatted(label);
		ImGui::PopStyleColor();
		if (srv) {
			ImGui::Image((ImTextureID)srv, size);
			if (ImGui::IsItemClicked()) { m_previewSRV = srv; m_previewLabel = label; m_showPreview = true; }
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Click para ampliar: %s", label);
		}
		else placeholder(size, "Sin datos");
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
		};
	float w = ImGui::GetContentRegionAvail().x;
	ImVec2 wide(w, w * 0.5625f);
	ImVec2 square(w, w);
	section("Shadow Map (profundidad de la luz)", shadowMapSRV, square);
	section("Pre-Shadow Pass (sin sombras)", preShadowSRV, wide);
	section("Resultado final (viewport)", viewportSRV, wide);
	ImGui::End();
}

void GUI::drawLightingPanel(float* lightDir, float* lightColor) {
	ImGui::Begin("Lighting");
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.28f, 0.40f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.36f, 0.48f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.20f, 0.32f, 1.0f));
	if (ImGui::Button("  Reset Scene  ")) {
		m_resetRequested = true;
		m_deferredDebugViewMode = 0;
		m_visualizeDeferredShadowFactor = false;
	}
	ImGui::PopStyleColor(3);
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("Restaura transforms, luz y camara a sus valores originales");
	ImGui::Separator();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.55f, 1.0f, 1.0f));
	ImGui::TextUnformatted("Luz direccional principal");
	ImGui::PopStyleColor();
	ImGui::Spacing();
	if (lightDir)   vec3Control("Direccion", lightDir, 0.0f, 90.0f);
	if (lightColor) vec3Control("Color", lightColor, 1.0f, 90.0f);
	ImGui::End();
}

void GUI::drawStatsPanel(float deltaTime, unsigned int drawCalls) {
	ImGui::Begin("Performance");
	static float history[120] = {};
	static int idx = 0;
	static float accum = 0.0f; static int frames = 0;
	static float fps = 0.0f; static float ms = 0.0f;

	float dtMs = deltaTime * 1000.0f;
	history[idx] = dtMs; idx = (idx + 1) % IM_ARRAYSIZE(history);
	accum += deltaTime; frames++;
	if (accum >= 0.25f) { fps = frames / accum; ms = (accum / frames) * 1000.0f; accum = 0.0f; frames = 0; }

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.55f, 1.0f, 1.0f));
	ImGui::SetWindowFontScale(1.7f);
	ImGui::Text("%.0f FPS", fps);
	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleColor();
	ImGui::SameLine();
	ImGui::TextDisabled("  %.2f ms", ms);

	ImGui::Spacing();
	ImGui::PlotLines("##frametimes", history, IM_ARRAYSIZE(history), idx,
		"Frame time (ms)", 0.0f, 33.3f, ImVec2(ImGui::GetContentRegionAvail().x, 80.0f));

	ImGui::Spacing(); ImGui::Separator();

	// --- Metricas del frame ---
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.72f, 0.98f, 1.0f));
	ImGui::Text("Draw calls:");
	ImGui::PopStyleColor();
	ImGui::SameLine();
	ImGui::Text("%u", drawCalls);

	ImGui::TextDisabled("Viewport: %.0f x %.0f", m_viewportSize.x, m_viewportSize.y);
	ImGui::End();
}


void GUI::drawConsolePanel() {
	ImGui::Begin("Console");
	if (ImGui::Button("Clear")) Logger::get().clear();
	ImGui::SameLine();
	ImGui::Checkbox("Info", &m_logShowInfo); ImGui::SameLine();
	ImGui::Checkbox("Warning", &m_logShowWarning); ImGui::SameLine();
	ImGui::Checkbox("Error", &m_logShowError); ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &m_logAutoScroll); ImGui::SameLine();
	m_logFilter.Draw("Filter", 160.0f);
	ImGui::Separator();
	ImGui::BeginChild("ConsoleScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	std::vector<LogEntry> entries = Logger::get().snapshot();
	for (const LogEntry& e : entries) {
		if (e.level == LogLevel::Info && !m_logShowInfo) continue;
		if (e.level == LogLevel::Warning && !m_logShowWarning) continue;
		if (e.level == LogLevel::Error && !m_logShowError) continue;
		if (!m_logFilter.PassFilter(e.message.c_str())) continue;
		ImVec4 col; const char* tag;
		switch (e.level) {
		case LogLevel::Error:   col = ImVec4(0.95f, 0.40f, 0.40f, 1.0f); tag = "[ERROR] "; break;
		case LogLevel::Warning: col = ImVec4(0.95f, 0.78f, 0.30f, 1.0f); tag = "[WARN]  "; break;
		default:                col = ImVec4(0.80f, 0.78f, 0.90f, 1.0f); tag = "[INFO]  "; break;
		}
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::TextUnformatted(tag); ImGui::SameLine();
		ImGui::TextUnformatted(e.message.c_str());
		ImGui::PopStyleColor();
	}
	if (m_logAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) ImGui::SetScrollHereY(1.0f);
	ImGui::EndChild();
	ImGui::End();
}

void GUI::drawTexturePreview() {
	if (!m_showPreview) return;
	ImGui::SetNextWindowSize(ImVec2(720.0f, 480.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Texture Preview", &m_showPreview)) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.55f, 1.0f, 1.0f));
		ImGui::TextUnformatted(m_previewLabel.c_str());
		ImGui::PopStyleColor();
		ImGui::Separator();
		if (m_previewSRV) {
			ImVec2 avail = ImGui::GetContentRegionAvail();
			if (avail.x < 16.0f) avail.x = 16.0f;
			if (avail.y < 16.0f) avail.y = 16.0f;
			ImGui::Image((ImTextureID)m_previewSRV, avail);
		}
	}
	ImGui::End();
}

void GUI::drawContentBrowser(const std::vector<AssetThumb>& textureThumbs) {
	ImGui::Begin("Content");

	if (ImGui::BeginTabBar("##ContentTabs")) {

		// ---- MODELS ----
		if (ImGui::BeginTabItem("Models")) {
			std::vector<std::string> models;
			WIN32_FIND_DATAA fd;
			HANDLE h = FindFirstFileA("Assets\\Models\\*", &fd);
			if (h != INVALID_HANDLE_VALUE) {
				do {
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
					std::string n = fd.cFileName;
					std::string lo = n;
					for (char& c : lo) if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
					if (lo.size() >= 4 && (lo.compare(lo.size() - 4, 4, ".fbx") == 0 ||
						lo.compare(lo.size() - 4, 4, ".obj") == 0))
						models.push_back(n);
				} while (FindNextFileA(h, &fd));
				FindClose(h);
			}

			if (models.empty()) ImGui::TextDisabled("No hay modelos en Assets/Models");

			const float cell = 90.0f;
			float availW = ImGui::GetContentRegionAvail().x;
			int perRow = (int)(availW / (cell + 10.0f)); if (perRow < 1) perRow = 1;
			int col = 0;
			for (const std::string& m : models) {
				ImGui::PushID(m.c_str());
				ImGui::BeginGroup();
				ImGui::Button("FBX/OBJ", ImVec2(cell, cell));
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s(doble click para instanciar)", m.c_str());
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						m_assetSpawnPath = "Assets/Models/" + m;
						m_assetSpawnRequested = true;
					}
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cell);
				ImGui::TextWrapped("%s", m.c_str());
				ImGui::PopTextWrapPos();
				if (ImGui::SmallButton("Spawn")) {
					m_assetSpawnPath = "Assets/Models/" + m;
					m_assetSpawnRequested = true;
				}
				ImGui::EndGroup();
				ImGui::PopID();
				if (++col < perRow) ImGui::SameLine(); else col = 0;
			}
			ImGui::EndTabItem();
		}

		// ---- TEXTURES ----
		if (ImGui::BeginTabItem("Textures")) {
			if (textureThumbs.empty()) ImGui::TextDisabled("No hay texturas cargadas");
			const float cell = 84.0f;
			float availW = ImGui::GetContentRegionAvail().x;
			int perRow = (int)(availW / (cell + 10.0f)); if (perRow < 1) perRow = 1;
			int col = 0;
			for (const AssetThumb& t : textureThumbs) {
				ImGui::PushID(t.name.c_str());
				ImGui::BeginGroup();
				if (t.srv) ImGui::Image((ImTextureID)t.srv, ImVec2(cell, cell));
				else       ImGui::Dummy(ImVec2(cell, cell));
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", t.name.c_str());
				ImGui::EndGroup();
				ImGui::PopID();
				if (++col < perRow) ImGui::SameLine(); else col = 0;
			}
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGui::End();
}

void GUI::drawSelectionOutline(Camera& cam, const EU::Vector3& mn, const EU::Vector3& mx, const XMMATRIX& world) {
	if (!m_viewportDrawList) return;
	if (m_viewportSize.x < 16.0f || m_viewportSize.y < 16.0f) return;

	XMMATRIX vp = cam.getView() * cam.getProj();

	ImVec2 pts[8];
	bool valid[8];
	for (int c = 0; c < 8; ++c) {
		float cx = (c & 1) ? mx.x : mn.x;
		float cy = (c & 2) ? mx.y : mn.y;
		float cz = (c & 4) ? mx.z : mn.z;
		XMVECTOR worldC = XMVector3TransformCoord(XMVectorSet(cx, cy, cz, 1.0f), world);
		XMVECTOR clip = XMVector4Transform(
			XMVectorSet(XMVectorGetX(worldC), XMVectorGetY(worldC), XMVectorGetZ(worldC), 1.0f), vp);
		float w = XMVectorGetW(clip);
		if (w <= 0.0001f) { valid[c] = false; pts[c] = ImVec2(0, 0); continue; }
		float ndcx = XMVectorGetX(clip) / w;
		float ndcy = XMVectorGetY(clip) / w;
		float sx = m_viewportPos.x + (ndcx * 0.5f + 0.5f) * m_viewportSize.x;
		float sy = m_viewportPos.y + (1.0f - (ndcy * 0.5f + 0.5f)) * m_viewportSize.y;
		pts[c] = ImVec2(sx, sy);
		valid[c] = true;
	}

	static const int edges[12][2] = {
		{0,1},{2,3},{4,5},{6,7},   // aristas en X
		{0,2},{1,3},{4,6},{5,7},   // aristas en Y
		{0,4},{1,5},{2,6},{3,7}    // aristas en Z
	};

	// Halo (linea gruesa oscura) + linea de acento encima = se ve mas pro
	for (int e = 0; e < 12; ++e) {
		int a = edges[e][0], b = edges[e][1];
		if (valid[a] && valid[b]) {
			m_viewportDrawList->AddLine(pts[a], pts[b], IM_COL32(0, 0, 0, 160), 4.0f);
			m_viewportDrawList->AddLine(pts[a], pts[b], IM_COL32(190, 140, 255, 240), 2.0f);
		}
	}
}

