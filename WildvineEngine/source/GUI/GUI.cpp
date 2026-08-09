#include "GUI/GUI.h"
#include "Viewport.h"
#include "Window.h"
#include "Device.h"
#include "DeviceContext.h"
#include "MeshComponent.h"
#include "ECS/Actor.h"
#include "EngineUtilities/Utilities/Camera.h"
#include "ECS/MeshRendererComponent.h"
#include "ECS/LightComponent.h"
#include "ECS/ParticleEmitterComponent.h"
#include "ECS/Transform.h"
#include "Rendering/Frustum.h"
#include "Rendering/Octree.h"
#include "SceneGraph/HierarchyComponent.h"
#include <functional>
#include <string>
#include <vector>


static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);

static const ImVec4 kBrand = ImVec4(0.12f, 0.72f, 0.48f, 1.0f);
static const ImVec4 kAccent = ImVec4(0.08f, 0.43f, 0.82f, 1.0f);
static const ImVec4 kAccentHi = ImVec4(0.24f, 0.65f, 1.00f, 1.0f);

void GUI::awake() {}

void GUI::init(Window& window, Device& device, DeviceContext& deviceContext) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = "WildvineLayout.ini";
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
	selectedActorIndex = -1;
}

void GUI::appleLiquidStyle(float opacity, ImVec4 accent) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Interfaz de editor profesional: superficies oscuras, bordes discretos
    // y azul para seleccion/interaccion. El verde se conserva como marca.
    style.WindowRounding = 2.0f;
    style.ChildRounding = 3.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.TabRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.ScrollbarRounding = 3.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(7.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 15.0f;
    style.ScrollbarSize = 11.0f;
    style.GrabMinSize = 9.0f;
    style.WindowTitleAlign = ImVec2(0.02f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;

    const ImVec4 base0 = ImVec4(0.035f, 0.043f, 0.052f, opacity);
    const ImVec4 base1 = ImVec4(0.060f, 0.072f, 0.084f, opacity);
    const ImVec4 base2 = ImVec4(0.085f, 0.102f, 0.118f, opacity);
    const ImVec4 base3 = ImVec4(0.120f, 0.142f, 0.162f, opacity);
    const ImVec4 text = ImVec4(0.90f, 0.92f, 0.94f, 1.0f);
    const ImVec4 textMuted = ImVec4(0.48f, 0.53f, 0.59f, 1.0f);
    const ImVec4 border = ImVec4(0.14f, 0.17f, 0.21f, 0.95f);

    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = textMuted;
    colors[ImGuiCol_WindowBg] = base0;
    colors[ImGuiCol_ChildBg] = ImVec4(0.028f, 0.034f, 0.041f, 0.82f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.042f, 0.051f, 0.061f, 0.99f);
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

    colors[ImGuiCol_FrameBg] = base1;
    colors[ImGuiCol_FrameBgHovered] = base2;
    colors[ImGuiCol_FrameBgActive] = base3;

    colors[ImGuiCol_TitleBg] = ImVec4(0.028f, 0.034f, 0.041f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.045f, 0.055f, 0.066f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.028f, 0.034f, 0.041f, 1.0f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.025f, 0.030f, 0.037f, 1.0f);

    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.018f, 0.022f, 0.027f, 0.75f);
    colors[ImGuiCol_ScrollbarGrab] = base2;
    colors[ImGuiCol_ScrollbarGrabHovered] = base3;
    colors[ImGuiCol_ScrollbarGrabActive] = accent;

    colors[ImGuiCol_CheckMark] = kAccentHi;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = kAccentHi;

    colors[ImGuiCol_Button] = base1;
    colors[ImGuiCol_ButtonHovered] = base2;
    colors[ImGuiCol_ButtonActive] = ImVec4(accent.x, accent.y, accent.z, 0.90f);

    colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.22f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.40f);
    colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.58f);

    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = kAccentHi;

    colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.18f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.45f);
    colors[ImGuiCol_ResizeGripActive] = kAccentHi;

    colors[ImGuiCol_Tab] = ImVec4(0.046f, 0.055f, 0.066f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(accent.x, accent.y, accent.z, 0.42f);
    colors[ImGuiCol_TabActive] = ImVec4(0.075f, 0.116f, 0.164f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.036f, 0.043f, 0.052f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.055f, 0.075f, 0.098f, 1.0f);

    colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.50f);
    colors[ImGuiCol_DockingEmptyBg] = base0;
    colors[ImGuiCol_PlotLines] = kAccentHi;
    colors[ImGuiCol_PlotHistogram] = accent;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.32f);
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
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);

    // Atajos de archivo con deteccion por flanco para evitar ejecutar
    // la misma accion durante varios frames mientras la tecla sigue pulsada.
    {
        const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool nNow = (GetAsyncKeyState('N') & 0x8000) != 0;
        const bool oNow = (GetAsyncKeyState('O') & 0x8000) != 0;
        const bool sNow = (GetAsyncKeyState('S') & 0x8000) != 0;
        static bool nPrevious = false;
        static bool oPrevious = false;
        static bool sPrevious = false;

        if (!io.WantTextInput && ctrl) {
            if (nNow && !nPrevious) requestSceneAction(1);
            if (oNow && !oPrevious) requestSceneAction(2);
            if (sNow && !sPrevious) {
                if (shift) m_saveSceneAsRequested = true;
                else m_saveSceneRequested = true;
            }
        }

        nPrevious = nNow;
        oPrevious = oNow;
        sPrevious = sNow;
    }

    drawStudioTopRibbon();
    drawEditorDockspace();
    drawEditorStatusBar();
    closeApp();
    drawUnsavedChangesPopup();
    drawGizmoToolbar();
}

void GUI::requestSceneAction(int action) {
    if (action <= 0) return;

    if (m_sceneDirty) {
        m_pendingSceneAction = action;
        m_showUnsavedChangesPopup = true;
        return;
    }

    m_pendingSceneAction = action;
    dispatchPendingSceneAction();
}

void GUI::dispatchPendingSceneAction() {
    switch (m_pendingSceneAction) {
    case 1: m_newSceneRequested = true; break;
    case 2: m_openSceneRequested = true; break;
    case 3: m_recoverSceneRequested = true; break;
    default: break;
    }
    m_pendingSceneAction = 0;
}

void GUI::drawUnsavedChangesPopup() {
    if (m_showUnsavedChangesPopup) {
        ImGui::OpenPopup("Cambios sin guardar");
        m_showUnsavedChangesPopup = false;
    }

    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(
        center,
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(
        "Cambios sin guardar",
        nullptr,
        ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::TextUnformatted("La escena actual tiene cambios sin guardar.");
        ImGui::TextUnformatted("Guardala antes de continuar para no perder trabajo.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
        if (ImGui::Button("Guardar primero", ImVec2(140.0f, 0.0f))) {
            m_saveSceneRequested = true;
            m_pendingSceneAction = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("Continuar sin guardar", ImVec2(175.0f, 0.0f))) {
            dispatchPendingSceneAction();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancelar", ImVec2(100.0f, 0.0f))) {
            m_pendingSceneAction = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
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
	if (show_exit_popup) {
		ImGui::OpenPopup("Cerrar Wildvine Studio");
		show_exit_popup = false;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Cerrar Wildvine Studio", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (m_sceneDirty) {
			ImGui::TextUnformatted("La escena tiene cambios sin guardar.");
			ImGui::TextUnformatted("Si cierras ahora, esos cambios se perderan.");
		}
		else {
			ImGui::TextUnformatted("Se cerrara Wildvine Engine Editor.");
		}
		ImGui::TextUnformatted("Deseas continuar?");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (m_sceneDirty) {
			if (ImGui::Button("Guardar primero", ImVec2(135, 0))) {
				m_saveSceneRequested = true;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
		}

		ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
		if (ImGui::Button("Cerrar", ImVec2(120, 0))) {
			exit(0);
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopStyleColor();

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancelar", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void GUI::inspectorGeneral(EU::TSharedPointer<Actor> actor) {
    if (!m_showInspector) return;
    ImGui::Begin("Propiedades", &m_showInspector);
    if (!actor) {
        ImGui::TextDisabled("Ningun objeto seleccionado");
        ImGui::End();
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.055f, 0.075f, 0.078f, 1.0f));
    ImGui::BeginChild("HeaderRegion", ImVec2(0, 78), true);

    bool active = actor->isActive();
    if (ImGui::Checkbox("##ActorActive", &active)) {
        actor->setActive(active);
    }
    ImGui::SameLine();

    char objectName[128] = {};
    strcpy_s(objectName, sizeof(objectName), actor->getName().c_str());
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("##ObjectName", objectName, IM_ARRAYSIZE(objectName))) {
        actor->setName(std::string(objectName));
    }

    ImGui::TextDisabled("ID: %d", actor->getId());
    ImGui::SameLine();

    auto lightComponent = actor->getComponent<LightComponent>();
    auto meshRenderer = actor->getComponent<MeshRendererComponent>();
    auto particleEmitter = actor->getComponent<ParticleEmitterComponent>();
    if (particleEmitter) {
        ImGui::TextDisabled("  Tipo: Particulas");
    }
    else if (lightComponent) {
        ImGui::TextDisabled("  Tipo: Luz");
    }
    else if (meshRenderer) {
        ImGui::TextDisabled("  Tipo: Modelo");
    }
    else {
        ImGui::TextDisabled("  Tipo: Objeto");
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        inspectorContainer(actor);
    }

    if (lightComponent &&
        ImGui::CollapsingHeader("Luz", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);

        LightData& light = lightComponent->getLightData();
        bool enabled = lightComponent->isEnabled();
        if (ImGui::Checkbox("Encendida", &enabled)) {
            lightComponent->setEnabled(enabled);
        }

        const char* lightTypes[] = {
            "Direccional",
            "Puntual",
            "Spotlight"
        };
        int lightType = static_cast<int>(lightComponent->getType());
        if (ImGui::Combo("Tipo", &lightType, lightTypes, IM_ARRAYSIZE(lightTypes))) {
            lightComponent->setType(static_cast<LightType>(lightType));
            lightComponent->setFollowTransformPosition(lightType != 0);
            lightComponent->setFollowTransformDirection(false);
        }

        float color[3] = { light.color.x, light.color.y, light.color.z };
        if (ImGui::ColorEdit3("Color", color)) {
            lightComponent->setColor(EU::Vector3(color[0], color[1], color[2]));
        }

        float intensity = light.intensity;
        if (ImGui::DragFloat("Intensidad", &intensity, 0.05f, 0.0f, 100.0f, "%.2f")) {
            lightComponent->setIntensity(intensity);
        }

        if (light.type != LightType::Directional) {
            float range = light.range;
            if (ImGui::DragFloat("Rango", &range, 0.1f, 0.01f, 1000.0f, "%.2f")) {
                lightComponent->setRange(range);
            }
        }

        if (light.type == LightType::Directional ||
            light.type == LightType::Spot) {
            float direction[3] = {
                light.direction.x,
                light.direction.y,
                light.direction.z
            };
            vec3Control("Direccion", direction, 0.0f, 90.0f);
            const EU::Vector3 editedDirection(direction[0], direction[1], direction[2]);
            if (!editedDirection.isNearlyZero() &&
                (editedDirection - light.direction).magnitudeSquared() > 0.000001f) {
                lightComponent->setDirection(editedDirection);
                lightComponent->setFollowTransformDirection(false);
            }
        }

        if (light.type == LightType::Spot) {
            float innerAngle = lightComponent->getInnerSpotAngle();
            float outerAngle = lightComponent->getOuterSpotAngle();
            bool angleChanged = false;
            angleChanged |= ImGui::SliderFloat("Angulo interior", &innerAngle, 1.0f, 85.0f, "%.1f grados");
            angleChanged |= ImGui::SliderFloat("Angulo exterior", &outerAngle, 2.0f, 89.0f, "%.1f grados");
            if (angleChanged) {
                lightComponent->setSpotAngles(innerAngle, outerAngle);
            }
        }

        bool castShadow = lightComponent->canCastShadow();
        if (ImGui::Checkbox("Proyectar sombras", &castShadow)) {
            lightComponent->setCastShadow(castShadow);
        }

        if (light.type == LightType::Directional ||
            light.type == LightType::Spot) {
            if (ImGui::Button("Apuntar al ultimo modelo seleccionado")) {
                m_aimLightRequested = true;
            }
        }

        ImGui::Unindent(10.0f);
    }

    if (particleEmitter &&
        ImGui::CollapsingHeader("Emisor de particulas", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);

        ParticleEmitterSettings& settings = particleEmitter->getSettings();

        bool emitterEnabled = particleEmitter->isEnabled();
        if (ImGui::Checkbox("Emisor activo", &emitterEnabled)) {
            particleEmitter->setEnabled(emitterEnabled);
        }

        const char* presetNames[] = {
            "Personalizado", "Humo", "Fuego", "Chispas", "Polvo"
        };
        int presetIndex = static_cast<int>(particleEmitter->getPreset());
        if (ImGui::Combo("Preset", &presetIndex,
            presetNames, IM_ARRAYSIZE(presetNames))) {
            particleEmitter->applyPreset(
                static_cast<ParticlePreset>(presetIndex));
        }

        ImGui::Spacing();
        if (particleEmitter->isPlaying()) {
            if (ImGui::Button("Pausar")) particleEmitter->pause();
        }
        else {
            if (ImGui::Button("Play")) particleEmitter->play();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reiniciar")) particleEmitter->restart();
        ImGui::SameLine();
        if (ImGui::Button("Burst")) particleEmitter->triggerBurst();
        ImGui::SameLine();
        if (ImGui::Button("Stop")) particleEmitter->stop();

        ImGui::Spacing();
        ImGui::TextDisabled("Emision");
        ImGui::Separator();

        const char* emissionModes[] = { "Continua", "Burst" };
        int emissionMode = static_cast<int>(settings.emissionMode);
        if (ImGui::Combo("Modo", &emissionMode,
            emissionModes, IM_ARRAYSIZE(emissionModes))) {
            settings.emissionMode =
                static_cast<ParticleEmissionMode>(emissionMode);
        }

        int maxParticles = static_cast<int>(settings.maxParticles);
        if (ImGui::SliderInt("Max particulas", &maxParticles, 1,
            static_cast<int>(ParticleEmitterComponent::kGpuParticleCapacity))) {
            settings.maxParticles = static_cast<unsigned int>(maxParticles);
        }

        if (settings.emissionMode == ParticleEmissionMode::Continuous) {
            ImGui::DragFloat("Particulas / segundo", &settings.spawnRate,
                0.5f, 0.0f, 1000.0f, "%.1f");
            ImGui::Checkbox("Loop", &settings.loop);
            if (!settings.loop) {
                ImGui::DragFloat("Duracion", &settings.duration,
                    0.1f, 0.05f, 120.0f, "%.2f s");
            }
        }
        else {
            int burstCount = static_cast<int>(settings.burstCount);
            if (ImGui::SliderInt("Cantidad Burst", &burstCount, 1,
                static_cast<int>(ParticleEmitterComponent::kGpuParticleCapacity))) {
                settings.burstCount = static_cast<unsigned int>(burstCount);
            }
        }

        ImGui::DragFloat("Vida minima", &settings.lifetimeMin,
            0.05f, 0.01f, 30.0f, "%.2f s");
        ImGui::DragFloat("Vida maxima", &settings.lifetimeMax,
            0.05f, 0.01f, 30.0f, "%.2f s");

        ImGui::Spacing();
        ImGui::TextDisabled("Movimiento");
        ImGui::Separator();

        float direction[3] = {
            settings.direction.x,
            settings.direction.y,
            settings.direction.z
        };
        if (ImGui::DragFloat3("Direccion", direction, 0.02f, -1.0f, 1.0f, "%.2f")) {
            settings.direction = EU::Vector3(
                direction[0], direction[1], direction[2]);
        }

        ImGui::DragFloat("Velocidad minima", &settings.speedMin,
            0.05f, 0.0f, 50.0f, "%.2f");
        ImGui::DragFloat("Velocidad maxima", &settings.speedMax,
            0.05f, 0.0f, 50.0f, "%.2f");
        ImGui::SliderFloat("Dispersion", &settings.spread,
            0.0f, 1.0f, "%.2f");
        ImGui::DragFloat("Radio de emision", &settings.spawnRadius,
            0.01f, 0.0f, 20.0f, "%.2f");
        ImGui::DragFloat("Gravedad Y", &settings.gravityY,
            0.05f, -30.0f, 30.0f, "%.2f");

        ImGui::Spacing();
        ImGui::TextDisabled("Apariencia");
        ImGui::Separator();

        ImGui::DragFloat("Tamano inicial", &settings.startSize,
            0.01f, 0.001f, 20.0f, "%.3f");
        ImGui::DragFloat("Tamano final", &settings.endSize,
            0.01f, 0.001f, 20.0f, "%.3f");

        float color[4] = {
            settings.color.x,
            settings.color.y,
            settings.color.z,
            settings.color.w
        };
        if (ImGui::ColorEdit4("Color / Alpha", color)) {
            settings.color = XMFLOAT4(
                color[0], color[1], color[2], color[3]);
        }

        ImGui::Checkbox("Blend aditivo", &settings.additiveBlend);
        ImGui::DragFloat("Emisivo", &settings.emissiveStrength,
            0.05f, 0.0f, 20.0f, "%.2f");
        ImGui::SliderFloat("Recorte Alpha", &settings.alphaCutoff,
            0.0f, 0.50f, "%.3f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Elimina el fondo transparente del PNG.\n"
                "Bajalo para bordes mas suaves; subelo si aun ves el cuadro.");
        }

        char texturePath[320] = {};
        strcpy_s(texturePath, sizeof(texturePath),
            particleEmitter->getTexturePath().c_str());
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("Textura", texturePath,
            IM_ARRAYSIZE(texturePath), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (particleEmitter->setTexturePath(texturePath)) {
                m_statusMessage = "Textura de particulas cargada";
            }
            else {
                m_statusMessage = "Textura no encontrada; se usa fallback";
            }
        }
        ImGui::TextDisabled("Pulsa Enter para recargar la ruta.");
        if (particleEmitter->hasValidParticleTexture()) {
            ImGui::TextColored(
                ImVec4(0.30f, 0.90f, 0.45f, 1.0f),
                "Textura: OK");
            const std::string& resolved =
                particleEmitter->getResolvedTexturePath();
            if (!resolved.empty() && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", resolved.c_str());
            }
        }
        else {
            ImGui::TextColored(
                ImVec4(1.00f, 0.45f, 0.30f, 1.0f),
                "Textura: FALLBACK (revisa Assets/Textures/Particles)");
        }

        particleEmitter->sanitizeSettings();

        ImGui::Spacing();
        ImGui::TextDisabled("Estado");
        ImGui::Separator();
        ImGui::Text("Activas: %u / %u",
            particleEmitter->getActiveParticleCount(),
            particleEmitter->getCapacity());
        ImGui::Text("Spawn este frame: %u",
            particleEmitter->getSpawnedThisFrame());
        ImGui::TextDisabled("Simulacion: %.3f ms | Billboards: %.3f ms",
            particleEmitter->getLastSimulationTimeMs(),
            particleEmitter->getLastBillboardTimeMs());

        ImGui::Unindent(10.0f);
    }

    if (meshRenderer &&
        ImGui::CollapsingHeader("Renderizado", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10.0f);

        bool visible = meshRenderer->isVisible();
        if (ImGui::Checkbox("Visible", &visible)) {
            meshRenderer->setVisible(visible);
        }

        bool castShadow = meshRenderer->canCastShadow();
        if (ImGui::Checkbox("Proyectar sombras", &castShadow)) {
            actor->setCastShadow(castShadow);
        }

        bool receiveShadow = meshRenderer->canReceiveShadow();
        if (ImGui::Checkbox("Recibir sombras", &receiveShadow)) {
            meshRenderer->setReceiveShadow(receiveShadow);
        }

        bool selectable = meshRenderer->isSelectable();
        if (ImGui::Checkbox("Seleccionable", &selectable)) {
            meshRenderer->setSelectable(selectable);
        }

        ImGui::TextDisabled("Materiales: %u",
            static_cast<unsigned int>(meshRenderer->getMaterialCount()));
        ImGui::Unindent(10.0f);
    }

    ImGui::End();
}

void GUI::inspectorContainer(EU::TSharedPointer<Actor> actor) {
    if (!actor) {
        return;
    }

    auto transform = actor->getComponent<Transform>();
    if (!transform) {
        return;
    }

    float position[3] = {
        transform->getPosition().x,
        transform->getPosition().y,
        transform->getPosition().z
    };
    float rotation[3] = {
        transform->getRotation().x,
        transform->getRotation().y,
        transform->getRotation().z
    };
    float scale[3] = {
        transform->getScale().x,
        transform->getScale().y,
        transform->getScale().z
    };

    const EU::Vector3 previousPosition = transform->getPosition();
    const EU::Vector3 previousRotation = transform->getRotation();
    const EU::Vector3 previousScale = transform->getScale();

    vec3Control("Position", position, 0.0f, 75.0f);
    vec3Control("Rotation", rotation, 0.0f, 75.0f);
    vec3Control("Scale", scale, 1.0f, 75.0f);

    const EU::Vector3 newPosition(position[0], position[1], position[2]);
    const EU::Vector3 newRotation(rotation[0], rotation[1], rotation[2]);
    const EU::Vector3 newScale(scale[0], scale[1], scale[2]);

    if ((newPosition - previousPosition).magnitudeSquared() > 0.000001f) {
        transform->setPosition(newPosition);
    }
    if ((newRotation - previousRotation).magnitudeSquared() > 0.000001f) {
        transform->setRotation(newRotation);
    }
    if ((newScale - previousScale).magnitudeSquared() > 0.000001f) {
        transform->setScale(newScale);
    }
}

void GUI::outliner(const std::vector<EU::TSharedPointer<Actor>>& actors) {
    m_cachedActorCount = static_cast<unsigned int>(actors.size());
    if (!m_showOutliner) return;
    ImGui::Begin("Escena", &m_showOutliner);

    ImGui::PushStyleColor(ImGuiCol_Text, kBrand);
    ImGui::TextUnformatted("JERARQUIA");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("%u objetos", m_cachedActorCount);
    ImGui::SameLine();
    if (ImGui::SmallButton("+")) ImGui::OpenPopup("##OutlinerCreate");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Crear luz u organizacion de estudio");
    if (ImGui::BeginPopup("##OutlinerCreate")) {
        if (ImGui::BeginMenu("Particulas")) {
            if (ImGui::MenuItem("Humo")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Smoke);
            if (ImGui::MenuItem("Fuego")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Fire);
            if (ImGui::MenuItem("Chispas")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Sparks);
            if (ImGui::MenuItem("Polvo")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Dust);
            if (ImGui::MenuItem("Personalizado")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Custom);
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Luz direccional")) m_createDirectionalLightRequested = true;
        if (ImGui::MenuItem("Luz puntual")) m_createPointLightRequested = true;
        if (ImGui::MenuItem("Spotlight")) m_createSpotLightRequested = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Rig de estudio")) m_createStudioRigRequested = true;
        ImGui::EndPopup();
    }
    ImGui::Separator();

    if (ImGui::Button("Mostrar todo")) {
        for (const auto& actor : actors) {
            if (actor.isNull()) continue;
            actor->setActive(true);

            EU::TSharedPointer<MeshRendererComponent> meshRenderer =
                actor->getComponent<MeshRendererComponent>();
            if (meshRenderer) meshRenderer->setVisible(true);

            EU::TSharedPointer<LightComponent> light =
                actor->getComponent<LightComponent>();
            if (light) light->setEnabled(true);

            EU::TSharedPointer<ParticleEmitterComponent> particles =
                actor->getComponent<ParticleEmitterComponent>();
            if (particles) particles->setEnabled(true);
        }
    }

    ImGui::SameLine();
    static ImGuiTextFilter filter;
    filter.Draw("Buscar", ImGui::GetContentRegionAvail().x - 8.0f);
    ImGui::Separator();

    // La raiz tambien funciona como destino: soltar aqui independiza el actor.
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.88f, 0.78f, 1.0f));
    ImGui::Selectable("[ESCENA] Raiz (suelta aqui para independizar)", false,
        0);
    ImGui::PopStyleColor();

    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload("WILDVINE_ACTOR_INDEX");
        if (payload && payload->DataSize == sizeof(int)) {
            const int childIndex = *static_cast<const int*>(payload->Data);
            m_reparentChildIndex = childIndex;
            m_reparentParentIndex = -1;
            m_reparentRequested = true;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Separator();

    std::unordered_map<Actor*, int> actorIndices;
    actorIndices.reserve(actors.size());
    for (int index = 0; index < static_cast<int>(actors.size()); ++index) {
        if (!actors[index].isNull()) {
            actorIndices[actors[index].get()] = index;
        }
    }

    std::vector<bool> drawn(actors.size(), false);
    std::vector<bool> recursionPath(actors.size(), false);

    std::function<void(int, bool)> drawActorNode;
    drawActorNode = [&](int index, bool drawChildren) {
        if (index < 0 || index >= static_cast<int>(actors.size())) return;
        if (actors[index].isNull()) return;
        if (drawChildren && drawn[index]) return;

        EU::TSharedPointer<Actor> actor = actors[index];
        const std::string actorName = actor->getName();

        if (filter.IsActive() && !filter.PassFilter(actorName.c_str())) {
            return;
        }

        if (drawChildren && recursionPath[index]) {
            ImGui::TextColored(
                ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
                "[!] Ciclo detectado en %s",
                actorName.c_str());
            return;
        }

        if (drawChildren) {
            drawn[index] = true;
            recursionPath[index] = true;
        }

        ImGui::PushID(actor->getId());

        EU::TSharedPointer<MeshRendererComponent> meshRenderer =
            actor->getComponent<MeshRendererComponent>();
        EU::TSharedPointer<LightComponent> light =
            actor->getComponent<LightComponent>();
        EU::TSharedPointer<ParticleEmitterComponent> particles =
            actor->getComponent<ParticleEmitterComponent>();
        EU::TSharedPointer<HierarchyComponent> hierarchy =
            actor->getComponent<HierarchyComponent>();

        bool itemEnabled = actor->isActive();
        if (meshRenderer) {
            itemEnabled = meshRenderer->isVisible();
            if (ImGui::Checkbox("##vis", &itemEnabled)) {
                meshRenderer->setVisible(itemEnabled);
            }
        }
        else if (light) {
            itemEnabled = light->isEnabled();
            if (ImGui::Checkbox("##vis", &itemEnabled)) {
                light->setEnabled(itemEnabled);
            }
        }
        else if (particles) {
            itemEnabled = particles->isEnabled();
            if (ImGui::Checkbox("##vis", &itemEnabled)) {
                particles->setEnabled(itemEnabled);
            }
        }
        else if (ImGui::Checkbox("##vis", &itemEnabled)) {
            actor->setActive(itemEnabled);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(itemEnabled ? "Activo" : "Desactivado");
        }
        ImGui::SameLine();

        bool hasActorChildren = false;
        if (drawChildren && hierarchy) {
            for (Entity* childEntity : hierarchy->m_children) {
                Actor* childActor = dynamic_cast<Actor*>(childEntity);
                if (childActor && actorIndices.find(childActor) != actorIndices.end()) {
                    hasActorChildren = true;
                    break;
                }
            }
        }

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!hasActorChildren) flags |= ImGuiTreeNodeFlags_Leaf;
        if (selectedActorIndex == index) flags |= ImGuiTreeNodeFlags_Selected;

        const char* typePrefix = particles ? "[P]" :
            (light ? "[L]" : (meshRenderer ? "[M]" : "[A]"));
        if (!itemEnabled) {
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                ImVec4(0.50f, 0.48f, 0.56f, 1.0f));
        }

        const bool nodeOpen = ImGui::TreeNodeEx(
            "##node",
            flags,
            "%s %s",
            typePrefix,
            actorName.c_str());

        if (!itemEnabled) ImGui::PopStyleColor();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            selectedActorIndex = index;
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload(
                "WILDVINE_ACTOR_INDEX",
                &index,
                sizeof(index));
            ImGui::Text("Mover: %s", actorName.c_str());
            ImGui::TextDisabled("Suelta sobre otro objeto o sobre la raiz");
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("WILDVINE_ACTOR_INDEX");
            if (payload && payload->DataSize == sizeof(int)) {
                const int childIndex =
                    *static_cast<const int*>(payload->Data);
                if (childIndex != index) {
                    m_reparentChildIndex = childIndex;
                    m_reparentParentIndex = index;
                    m_reparentRequested = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem()) {
            selectedActorIndex = index;
            ImGui::TextDisabled("Acciones del objeto");
            ImGui::Separator();

            if (ImGui::MenuItem("Duplicar", "Ctrl+D")) m_duplicateRequested = true;
            if (ImGui::MenuItem("Copiar", "Ctrl+C")) m_copyRequested = true;
            if (ImGui::MenuItem("Pegar", "Ctrl+V")) m_pasteRequested = true;

            ImGui::Separator();
            if (hierarchy && hierarchy->m_parent &&
                ImGui::MenuItem("Mover a la raiz")) {
                m_reparentChildIndex = index;
                m_reparentParentIndex = -1;
                m_reparentRequested = true;
            }

            if (ImGui::MenuItem("Aislar objeto")) {
                for (const auto& otherActor : actors) {
                    if (otherActor.isNull()) continue;
                    EU::TSharedPointer<MeshRendererComponent> otherRenderer =
                        otherActor->getComponent<MeshRendererComponent>();
                    if (otherRenderer) {
                        otherRenderer->setVisible(
                            otherActor.get() == actor.get());
                    }
                }
            }
            if (ImGui::MenuItem("Mostrar todo")) {
                for (const auto& otherActor : actors) {
                    if (otherActor.isNull()) continue;
                    otherActor->setActive(true);

                    EU::TSharedPointer<MeshRendererComponent> otherRenderer =
                        otherActor->getComponent<MeshRendererComponent>();
                    if (otherRenderer) otherRenderer->setVisible(true);

                    EU::TSharedPointer<LightComponent> otherLight =
                        otherActor->getComponent<LightComponent>();
                    if (otherLight) otherLight->setEnabled(true);
                }
            }

            if (meshRenderer) {
                const bool visible = meshRenderer->isVisible();
                if (ImGui::MenuItem(visible ? "Ocultar" : "Mostrar")) {
                    meshRenderer->setVisible(!visible);
                }
            }
            if (light) {
                const bool enabled = light->isEnabled();
                if (ImGui::MenuItem(enabled ? "Apagar luz" : "Encender luz")) {
                    light->setEnabled(!enabled);
                }
            }
            if (particles) {
                const bool enabled = particles->isEnabled();
                if (ImGui::MenuItem(enabled ? "Ocultar particulas" : "Mostrar particulas")) {
                    particles->setEnabled(!enabled);
                }
            }

            ImGui::Separator();
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            if (ImGui::MenuItem("Eliminar", "Supr")) {
                m_deleteRequested = true;
            }
            ImGui::PopStyleColor();
            ImGui::EndPopup();
        }

        if (nodeOpen) {
            EU::TSharedPointer<Transform> transform =
                actor->getComponent<Transform>();
            if (transform) {
                ImGui::TextDisabled(
                    "Pos local: %.1f, %.1f, %.1f",
                    transform->getPosition().x,
                    transform->getPosition().y,
                    transform->getPosition().z);
            }

            if (drawChildren && hierarchy) {
                for (Entity* childEntity : hierarchy->m_children) {
                    Actor* childActor = dynamic_cast<Actor*>(childEntity);
                    const auto childIterator = actorIndices.find(childActor);
                    if (childActor && childIterator != actorIndices.end()) {
                        drawActorNode(childIterator->second, true);
                    }
                }
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
        if (drawChildren) recursionPath[index] = false;
    };

    if (filter.IsActive()) {
        for (int index = 0; index < static_cast<int>(actors.size()); ++index) {
            drawActorNode(index, false);
        }
    }
    else {
        for (int index = 0; index < static_cast<int>(actors.size()); ++index) {
            if (actors[index].isNull()) continue;

            EU::TSharedPointer<HierarchyComponent> hierarchy =
                actors[index]->getComponent<HierarchyComponent>();
            Actor* parentActor = hierarchy
                ? dynamic_cast<Actor*>(hierarchy->m_parent)
                : nullptr;

            if (!parentActor || actorIndices.find(parentActor) == actorIndices.end()) {
                drawActorNode(index, true);
            }
        }

        // Fallback para nodos corruptos o ciclos que no tengan una raiz valida.
        for (int index = 0; index < static_cast<int>(actors.size()); ++index) {
            if (!drawn[index]) drawActorNode(index, true);
        }
    }

    ImGui::End();
}

void GUI::editTransform(Camera& cam, Window& window, EU::TSharedPointer<Actor> actor) {
	if (!actor) return;
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
	ImDrawList* gizmoDrawList = m_viewportDrawList
		? m_viewportDrawList
		: ImGui::GetForegroundDrawList();
	ImGuizmo::SetDrawlist(gizmoDrawList);
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

	// El gizmo comparte la lista de dibujo de ImGui. Sin un clip explicito,
	// sus lineas pueden aparecer sobre el Inspector, la Consola o Recursos.
	if (gizmoDrawList) {
		gizmoDrawList->PushClipRect(
			ImVec2(rectX, rectY),
			ImVec2(rectX + rectW, rectY + rectH),
			true);
	}

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

	if (gizmoDrawList) {
		gizmoDrawList->PopClipRect();
	}
}

void GUI::drawGizmoToolbar() {
    if (!m_showViewportOverlay) return;
    if (m_viewportSize.x < 80.0f || m_viewportSize.y < 80.0f) return;

    ImGui::SetNextWindowPos(
        ImVec2(m_viewportPos.x + 12.0f, m_viewportPos.y + 12.0f),
        ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 3.0f));

    if (ImGui::Begin("##WildvineViewportTools", nullptr, windowFlags)) {
        auto modeButton = [&](const char* label, ImGuizmo::OPERATION operation, const char* shortcut) {
            const bool active = (mCurrentGizmoOperation == operation);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kAccentHi);
            }

            if (ImGui::Button(label, ImVec2(30.0f, 28.0f))) {
                mCurrentGizmoOperation = operation;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s [%s]", label, shortcut);
            }

            if (active) ImGui::PopStyleColor(2);
        };

        modeButton("W", ImGuizmo::TRANSLATE, "W");
        ImGui::SameLine();
        modeButton("E", ImGuizmo::ROTATE, "E");
        ImGui::SameLine();
        modeButton("R", ImGuizmo::SCALE, "R");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        if (ImGui::Button(
            mCurrentGizmoMode == ImGuizmo::WORLD ? "Mundo" : "Local",
            ImVec2(52.0f, 28.0f))) {
            mCurrentGizmoMode =
                (mCurrentGizmoMode == ImGuizmo::WORLD)
                ? ImGuizmo::LOCAL
                : ImGuizmo::WORLD;
        }

        ImGui::SameLine();
        if (ImGui::Button(m_snapEnabled ? "Snap ON" : "Snap", ImVec2(56.0f, 28.0f))) {
            m_snapEnabled = !m_snapEnabled;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Ajuste a rejilla. Ctrl activa ajuste temporal");
        }

        ImGui::SameLine();
        if (ImGui::Button("F", ImVec2(30.0f, 28.0f))) {
            m_focusRequested = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Enfocar objeto seleccionado [F]");
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void GUI::drawStudioTopRibbon() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float menuBarHeight = 28.0f;
    const float ribbonHeight = 56.0f;

    ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, menuBarHeight), ImGuiCond_Always);

    ImGuiWindowFlags menuFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.020f, 0.025f, 0.031f, 1.0f));

    if (ImGui::Begin("##WildvineMenuBar", nullptr, menuFlags)) {
        if (ImGui::BeginMenuBar()) {
            ImGui::PushStyleColor(ImGuiCol_Text, kBrand);
            ImGui::TextUnformatted("WILDVINE");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextDisabled("ENGINE EDITOR");
            ImGui::Separator();

            if (ImGui::BeginMenu("Archivo")) {
                if (ImGui::MenuItem("Nueva escena", "Ctrl+N")) {
                    requestSceneAction(1);
                }
                if (ImGui::MenuItem("Abrir escena...", "Ctrl+O")) {
                    requestSceneAction(2);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Guardar", "Ctrl+S")) {
                    m_saveSceneRequested = true;
                }
                if (ImGui::MenuItem("Guardar como...", "Ctrl+Shift+S")) {
                    m_saveSceneAsRequested = true;
                }
                if (m_recoveryAvailable) {
                    ImGui::Separator();
                    if (ImGui::MenuItem("Recuperar autoguardado")) {
                        requestSceneAction(3);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Cerrar editor")) show_exit_popup = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Editar")) {
                if (ImGui::MenuItem("Deshacer", "Ctrl+Z")) m_undoRequested = true;
                if (ImGui::MenuItem("Rehacer", "Ctrl+Y")) m_redoRequested = true;
                ImGui::Separator();
                if (ImGui::MenuItem("Copiar", "Ctrl+C")) m_copyRequested = true;
                if (ImGui::MenuItem("Pegar", "Ctrl+V")) m_pasteRequested = true;
                if (ImGui::MenuItem("Duplicar", "Ctrl+D")) m_duplicateRequested = true;
                if (ImGui::MenuItem("Eliminar", "Supr")) m_deleteRequested = true;
                ImGui::Separator();
                if (ImGui::MenuItem("Guardar prefab")) m_savePrefabRequested = true;
                if (ImGui::MenuItem("Cargar prefab")) m_loadPrefabRequested = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Crear")) {
                if (ImGui::BeginMenu("Particulas")) {
                    if (ImGui::MenuItem("Humo")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Smoke);
                    if (ImGui::MenuItem("Fuego")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Fire);
                    if (ImGui::MenuItem("Chispas")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Sparks);
                    if (ImGui::MenuItem("Polvo")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Dust);
                    if (ImGui::MenuItem("Personalizado")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Custom);
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Luz direccional")) m_createDirectionalLightRequested = true;
                if (ImGui::MenuItem("Luz puntual")) m_createPointLightRequested = true;
                if (ImGui::MenuItem("Spotlight")) m_createSpotLightRequested = true;
                ImGui::Separator();
                if (ImGui::MenuItem("Rig de estudio (3 luces)")) m_createStudioRigRequested = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Ventana")) {
                ImGui::MenuItem("Escena", nullptr, &m_showOutliner);
                ImGui::MenuItem("Propiedades", nullptr, &m_showInspector);
                ImGui::MenuItem("Recursos", nullptr, &m_showContentBrowser);
                ImGui::MenuItem("Consola", nullptr, &m_showConsole);
                ImGui::MenuItem("Rendimiento", nullptr, &m_showPerformance);
                ImGui::Separator();
                ImGui::MenuItem("Iluminacion", nullptr, &m_showLightingPanel);
                ImGui::MenuItem("Render", nullptr, &m_showRenderPanel);
                ImGui::MenuItem("Datos de Render", nullptr, &m_showGBufferPanel);
                ImGui::Separator();
                if (ImGui::MenuItem("Restablecer distribucion")) {
                    m_dockLayoutInitialized = false;
                    m_showOutliner = true;
                    m_showInspector = true;
                    m_showContentBrowser = true;
                    m_showConsole = true;
                    m_showPerformance = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Vista")) {
                ImGui::MenuItem("Mostrar rejilla", nullptr, &m_showGrid);
                ImGui::MenuItem("Overlay del viewport", nullptr, &m_showViewportOverlay);
                ImGui::MenuItem("Ajuste a rejilla", nullptr, &m_snapEnabled);
                ImGui::Separator();
                if (ImGui::MenuItem("Enfocar seleccionado", "F")) m_focusRequested = true;
                if (ImGui::MenuItem("Encuadrar escena")) m_fitRequested = true;
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Ayuda")) {
                ImGui::TextDisabled("Wildvine Engine Editor");
                ImGui::TextDisabled("DirectX 11 / ImGui");
                ImGui::Separator();
                ImGui::TextDisabled("W/E/R: mover, rotar y escalar");
                ImGui::TextDisabled("F: enfocar seleccionado");
                ImGui::EndMenu();
            }

            std::string sceneLabel = m_sceneDisplayName.empty()
                ? "Sin titulo"
                : m_sceneDisplayName;
            if (m_sceneDirty) sceneLabel += " *";
            const float sceneLabelWidth = ImGui::CalcTextSize(sceneLabel.c_str()).x;
            const float rightPosition = ImGui::GetWindowWidth() - sceneLabelWidth - 20.0f;
            if (rightPosition > ImGui::GetCursorPosX()) {
                ImGui::SetCursorPosX(rightPosition);
            }
            if (m_sceneDirty) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.72f, 0.28f, 1.0f));
                ImGui::TextUnformatted(sceneLabel.c_str());
                ImGui::PopStyleColor();
            }
            else {
                ImGui::TextDisabled("%s", sceneLabel.c_str());
            }

            ImGui::EndMenuBar();
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(viewport->Size.x, ribbonHeight),
        ImGuiCond_Always);

    ImGuiWindowFlags ribbonFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.043f, 0.052f, 0.063f, 1.0f));

    if (ImGui::Begin("##WildvineCommandBar", nullptr, ribbonFlags)) {
        auto actionButton = [&](const char* id, const char* label, const char* key, bool active) -> bool {
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
            const bool pressed = ImGui::Button(id, ImVec2(72.0f, 38.0f));
            const ImVec2 itemMin = ImGui::GetItemRectMin();
            const ImVec2 itemMax = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 labelSize = ImGui::CalcTextSize(label);
            const ImVec2 keySize = ImGui::CalcTextSize(key);
            const float centerX = (itemMin.x + itemMax.x) * 0.5f;
            drawList->AddText(
                ImVec2(centerX - labelSize.x * 0.5f, itemMin.y + 5.0f),
                ImGui::GetColorU32(ImGuiCol_Text), label);
            drawList->AddText(
                ImVec2(centerX - keySize.x * 0.5f, itemMin.y + 21.0f),
                ImGui::GetColorU32(ImGuiCol_TextDisabled), key);
            if (active) ImGui::PopStyleColor();
            return pressed;
        };

        auto divider = [&]() {
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
        };

        if (actionButton("##WV_Save", "Guardar", "Ctrl+S", false)) {
            m_saveSceneRequested = true;
        }

        divider();

        if (actionButton("##WV_Move", "Mover", "W",
            mCurrentGizmoOperation == ImGuizmo::TRANSLATE)) {
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        }
        ImGui::SameLine();
        if (actionButton("##WV_Rotate", "Rotar", "E",
            mCurrentGizmoOperation == ImGuizmo::ROTATE)) {
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        }
        ImGui::SameLine();
        if (actionButton("##WV_Scale", "Escalar", "R",
            mCurrentGizmoOperation == ImGuizmo::SCALE)) {
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        }

        divider();

        if (actionButton("##WV_Model", "Modelos", "Recursos", false)) {
            m_showContentBrowser = true;
            m_requestedContentTab = 0;
            ImGui::SetWindowFocus("Recursos");
        }
        ImGui::SameLine();
        if (actionButton("##WV_Particles", "Particulas", "Crear", false)) {
            ImGui::OpenPopup("##CrearParticulasPopup");
        }
        if (ImGui::BeginPopup("##CrearParticulasPopup")) {
            if (ImGui::MenuItem("Humo")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Smoke);
            if (ImGui::MenuItem("Fuego")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Fire);
            if (ImGui::MenuItem("Chispas")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Sparks);
            if (ImGui::MenuItem("Polvo")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Dust);
            if (ImGui::MenuItem("Personalizado")) m_createParticlePresetRequested = static_cast<int>(ParticlePreset::Custom);
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if (actionButton("##WV_Light", "Luces", "Crear", false)) {
            ImGui::OpenPopup("##CrearLuzPopup");
        }
        if (ImGui::BeginPopup("##CrearLuzPopup")) {
            if (ImGui::MenuItem("Direccional")) m_createDirectionalLightRequested = true;
            if (ImGui::MenuItem("Puntual")) m_createPointLightRequested = true;
            if (ImGui::MenuItem("Spotlight")) m_createSpotLightRequested = true;
            ImGui::Separator();
            if (ImGui::MenuItem("Rig de estudio")) m_createStudioRigRequested = true;
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if (actionButton("##WV_Material", "Inspector", "Material", false)) {
            m_showInspector = true;
            ImGui::SetWindowFocus("Propiedades");
        }

        divider();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.42f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.13f, 0.56f, 0.37f, 1.0f));
        if (actionButton("##WV_Focus", "Enfocar", "F", false)) {
            m_focusRequested = true;
            ImGui::SetWindowFocus("Vista 3D");
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();
        ImGui::Dummy(ImVec2(12.0f, 1.0f));
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, kBrand);
        ImGui::TextUnformatted("ESCENA");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("%s%s",
            m_sceneDisplayName.c_str(),
            m_sceneDirty ? " *" : "");
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

void GUI::drawViewportPanel(ID3D11ShaderResourceView* viewportSRV) {
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if (ImGui::Begin("Vista 3D", nullptr, flags)) {
        m_viewportDrawList = ImGui::GetWindowDrawList();

        const ImVec2 panelMin = ImGui::GetCursorScreenPos();
        ImVec2 panelSize = ImGui::GetContentRegionAvail();
        if (panelSize.x < 1.0f) panelSize.x = 1.0f;
        if (panelSize.y < 1.0f) panelSize.y = 1.0f;

        m_viewportPos = panelMin;
        m_viewportSize = panelSize;

        if (viewportSRV) {
            ImGui::Image((ImTextureID)viewportSRV, panelSize);
        }
        else {
            ImGui::InvisibleButton("##ViewportDropTarget", panelSize);
            const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
            m_viewportDrawList->AddRectFilled(panelMin, panelMax, IM_COL32(10, 13, 17, 255));
            m_viewportDrawList->AddText(
                ImVec2(panelMin.x + 16.0f, panelMin.y + 16.0f),
                IM_COL32(205, 220, 232, 255),
                "Vista 3D sin render");
        }

        m_viewportHovered = ImGui::IsItemHovered();
        m_viewportActive = ImGui::IsItemActive();
        m_viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("WILDVINE_MODEL_ASSET")) {
                const char* modelPath = static_cast<const char*>(payload->Data);
                if (modelPath && payload->DataSize > 1) {
                    m_assetSpawnPath = modelPath;
                    m_assetSpawnScreenPosition = ImGui::GetMousePos();
                    m_assetSpawnRequested = true;
                    m_statusMessage = "Importando modelo...";
                }
            }
            ImGui::EndDragDropTarget();
        }

        const ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);
        if (m_showViewportOverlay) {
            // Etiquetas de modo, similares a un editor 3D profesional.
            const ImVec2 chipMin(panelMin.x + 10.0f, panelMin.y + 10.0f);
            const ImVec2 chipMax(chipMin.x + 92.0f, chipMin.y + 24.0f);
            m_viewportDrawList->AddRectFilled(chipMin, chipMax, IM_COL32(23, 29, 37, 225), 3.0f);
            m_viewportDrawList->AddRect(chipMin, chipMax, IM_COL32(61, 76, 92, 220), 3.0f);
            m_viewportDrawList->AddText(
                ImVec2(chipMin.x + 9.0f, chipMin.y + 5.0f),
                IM_COL32(220, 228, 236, 255),
                "Perspectiva");

            const ImVec2 litMin(chipMax.x + 6.0f, chipMin.y);
            const ImVec2 litMax(litMin.x + 80.0f, litMin.y + 24.0f);
            m_viewportDrawList->AddRectFilled(litMin, litMax, IM_COL32(23, 29, 37, 225), 3.0f);
            m_viewportDrawList->AddRect(litMin, litMax, IM_COL32(61, 76, 92, 220), 3.0f);
            m_viewportDrawList->AddText(
                ImVec2(litMin.x + 10.0f, litMin.y + 5.0f),
                IM_COL32(220, 228, 236, 255),
                "Iluminado");

            char resolution[64] = {};
            sprintf_s(resolution, sizeof(resolution), "%.0f x %.0f", panelSize.x, panelSize.y);
            const ImVec2 resolutionSize = ImGui::CalcTextSize(resolution);
            m_viewportDrawList->AddText(
                ImVec2(panelMax.x - resolutionSize.x - 12.0f, panelMin.y + 15.0f),
                IM_COL32(165, 178, 192, 235),
                resolution);
        }

        if (m_viewportHovered) {
            m_viewportDrawList->AddRect(
                panelMin, panelMax,
                IM_COL32(44, 132, 224, 185),
                0.0f, 0, 1.5f);
        }

        if (const ImGuiPayload* payload = ImGui::GetDragDropPayload()) {
            if (payload->IsDataType("WILDVINE_MODEL_ASSET") && m_viewportHovered) {
                m_viewportDrawList->AddRect(
                    panelMin, panelMax,
                    IM_COL32(48, 182, 255, 255),
                    0.0f, 0, 3.0f);

                const ImVec2 boxMin(panelMin.x + 18.0f, panelMin.y + 46.0f);
                const ImVec2 boxMax(boxMin.x + 225.0f, boxMin.y + 32.0f);
                m_viewportDrawList->AddRectFilled(boxMin, boxMax, IM_COL32(18, 29, 40, 235), 4.0f);
                m_viewportDrawList->AddText(
                    ImVec2(boxMin.x + 10.0f, boxMin.y + 8.0f),
                    IM_COL32(112, 210, 255, 255),
                    "Soltar para colocar en la escena");
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void GUI::drawViewportGrid(Camera& cam) {
	if (!m_showGrid || !m_viewportDrawList) return;
	if (m_viewportSize.x < 16.0f || m_viewportSize.y < 16.0f) return;
	float view[16], proj[16], identity[16];
	ToFloatArray(cam.getView(), view);
	ToFloatArray(cam.getProj(), proj);
	ToFloatArray(XMMatrixIdentity(), identity);

	m_viewportDrawList->PushClipRect(
		m_viewportPos,
		ImVec2(
			m_viewportPos.x + m_viewportSize.x,
			m_viewportPos.y + m_viewportSize.y),
		true);
	ImGuizmo::SetDrawlist(m_viewportDrawList);
	ImGuizmo::SetRect(m_viewportPos.x, m_viewportPos.y, m_viewportSize.x, m_viewportSize.y);
	ImGuizmo::DrawGrid(view, proj, identity, m_gridSize);
	m_viewportDrawList->PopClipRect();
}


void GUI::drawFrustumCullingDebug(
    const std::vector<EU::TSharedPointer<Actor>>& actors,
    Camera& camera,
    const Frustum& debugFrustum,
    const Octree* octree) {

    if (!m_viewportDrawList ||
        m_viewportSize.x < 16.0f ||
        m_viewportSize.y < 16.0f ||
        (!m_showCullingBounds &&
         !m_showFrustumWireframe &&
         !m_showOctreeDebug)) {
        return;
    }

    const ImVec2 clipMin = m_viewportPos;
    const ImVec2 clipMax(
        m_viewportPos.x + m_viewportSize.x,
        m_viewportPos.y + m_viewportSize.y);

    const XMMATRIX viewProjection =
        camera.getView() * camera.getProj();

    auto projectWorld = [&](
        const EU::Vector3& point,
        ImVec2& output) -> bool {

        const XMVECTOR clip = XMVector4Transform(
            XMVectorSet(point.x, point.y, point.z, 1.0f),
            viewProjection);

        const float w = XMVectorGetW(clip);
        if (!std::isfinite(w) || w <= 0.0001f) {
            return false;
        }

        const float ndcX = XMVectorGetX(clip) / w;
        const float ndcY = XMVectorGetY(clip) / w;
        if (!std::isfinite(ndcX) || !std::isfinite(ndcY)) {
            return false;
        }

        output.x = m_viewportPos.x +
            (ndcX * 0.5f + 0.5f) * m_viewportSize.x;
        output.y = m_viewportPos.y +
            (1.0f - (ndcY * 0.5f + 0.5f)) * m_viewportSize.y;
        return true;
    };

    static const int edges[12][2] = {
        {0,1},{2,3},{4,5},{6,7},
        {0,2},{1,3},{4,6},{5,7},
        {0,4},{1,5},{2,6},{3,7}
    };

    auto drawWorldBox = [&](
        const EU::Vector3& localMin,
        const EU::Vector3& localMax,
        const XMMATRIX& world,
        ImU32 color,
        float thickness) {

        ImVec2 points[8]{};
        bool valid[8]{};

        for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
            const float x = (cornerIndex & 1) ? localMax.x : localMin.x;
            const float y = (cornerIndex & 2) ? localMax.y : localMin.y;
            const float z = (cornerIndex & 4) ? localMax.z : localMin.z;

            const XMVECTOR worldCorner = XMVector3TransformCoord(
                XMVectorSet(x, y, z, 1.0f),
                world);

            XMFLOAT3 worldPoint{};
            XMStoreFloat3(&worldPoint, worldCorner);
            valid[cornerIndex] = projectWorld(
                EU::Vector3(worldPoint.x, worldPoint.y, worldPoint.z),
                points[cornerIndex]);
        }

        for (int edgeIndex = 0; edgeIndex < 12; ++edgeIndex) {
            const int a = edges[edgeIndex][0];
            const int b = edges[edgeIndex][1];
            if (!valid[a] || !valid[b]) {
                continue;
            }

            m_viewportDrawList->AddLine(
                points[a], points[b],
                IM_COL32(0, 0, 0, 145),
                thickness + 2.0f);
            m_viewportDrawList->AddLine(
                points[a], points[b],
                color,
                thickness);
        }
    };

    m_viewportDrawList->PushClipRect(
        clipMin,
        clipMax,
        true);

    if (m_showCullingBounds) {
        for (const EU::TSharedPointer<Actor>& actor : actors) {
            if (actor.isNull() || !actor->isActive()) {
                continue;
            }

            EU::TSharedPointer<Transform> transform =
                actor->getComponent<Transform>();
            EU::TSharedPointer<MeshRendererComponent> renderer =
                actor->getComponent<MeshRendererComponent>();

            if (!transform ||
                !renderer ||
                !renderer->isVisible() ||
                !renderer->hasMesh()) {
                continue;
            }

            EU::Vector3 localMin;
            EU::Vector3 localMax;
            if (!renderer->getLocalBounds(localMin, localMax)) {
                continue;
            }

            const FrustumBoxResult result =
                debugFrustum.classifyBox(
                    localMin,
                    localMax,
                    transform->worldMatrix);

            if (result == FrustumBoxResult::Outside &&
                !m_showCulledBounds) {
                continue;
            }

            ImU32 color = IM_COL32(238, 196, 64, 225);
            if (result == FrustumBoxResult::Inside) {
                color = IM_COL32(52, 224, 116, 225);
            }
            else if (result == FrustumBoxResult::Intersecting) {
                color = IM_COL32(255, 174, 54, 235);
            }
            else if (result == FrustumBoxResult::Outside) {
                color = IM_COL32(242, 72, 72, 235);
            }

            drawWorldBox(
                localMin,
                localMax,
                transform->worldMatrix,
                color,
                1.5f);
        }
    }

    if (m_showFrustumWireframe) {
        std::array<EU::Vector3, 8> corners{};
        if (debugFrustum.getWorldCorners(corners)) {
            ImVec2 projected[8]{};
            bool valid[8]{};

            for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
                valid[cornerIndex] = projectWorld(
                    corners[cornerIndex],
                    projected[cornerIndex]);
            }

            for (int edgeIndex = 0; edgeIndex < 12; ++edgeIndex) {
                const int a = edges[edgeIndex][0];
                const int b = edges[edgeIndex][1];
                if (!valid[a] || !valid[b]) {
                    continue;
                }

                m_viewportDrawList->AddLine(
                    projected[a], projected[b],
                    IM_COL32(255, 255, 255, 190),
                    2.0f);
            }
        }
    }

    if (m_showOctreeDebug &&
        m_octreeEnabled &&
        octree &&
        octree->hasRoot()) {

        const std::vector<OctreeDebugBox>& debugBoxes =
            octree->getDebugBoxes();

        for (const OctreeDebugBox& box : debugBoxes) {
            ImU32 color = IM_COL32(80, 190, 255, 150);

            if (box.classification == FrustumBoxResult::Inside) {
                color = IM_COL32(70, 210, 175, 145);
            }
            else if (box.classification == FrustumBoxResult::Outside) {
                color = IM_COL32(245, 80, 80, 165);
            }
            else if (box.classification == FrustumBoxResult::Intersecting) {
                color = IM_COL32(75, 165, 255, 170);
            }

            drawWorldBox(
                box.bounds.minimum,
                box.bounds.maximum,
                XMMatrixIdentity(),
                color,
                box.leaf ? 1.0f : 1.35f);
        }
    }

    if (m_showCullingBounds ||
        m_showFrustumWireframe ||
        (m_showOctreeDebug && m_octreeEnabled)) {
        const ImVec2 legendMin(
            clipMin.x + 12.0f,
            clipMax.y - 30.0f);
        const ImVec2 legendMax(
            legendMin.x + 365.0f,
            legendMin.y + 22.0f);

        m_viewportDrawList->AddRectFilled(
            legendMin,
            legendMax,
            IM_COL32(12, 17, 22, 218),
            3.0f);
        m_viewportDrawList->AddText(
            ImVec2(legendMin.x + 8.0f, legendMin.y + 4.0f),
            IM_COL32(210, 220, 230, 245),
            m_showOctreeDebug && m_octreeEnabled
                ? "Culling: AABB verde/naranja/rojo | Octree azul/cian/rojo"
                : "Culling: verde dentro | naranja intersecta | rojo fuera");
    }

    m_viewportDrawList->PopClipRect();
}

void GUI::drawEditorDockspace() {
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    const float topOffset = 84.0f;
    const float statusBarHeight = 24.0f;

    const ImVec2 dockPos(
        mainViewport->Pos.x,
        mainViewport->Pos.y + topOffset);

    ImVec2 dockSize(
        mainViewport->Size.x,
        mainViewport->Size.y - topOffset - statusBarHeight);
    if (dockSize.y < 1.0f) dockSize.y = 1.0f;

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos(dockPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(dockSize, ImGuiCond_Always);
    ImGui::SetNextWindowViewport(mainViewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("##WildvineDockspaceHost", nullptr, windowFlags);

    const ImGuiID dockspaceId = ImGui::GetID("##WildvineEditorDockspace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (!m_dockLayoutInitialized) {
        m_dockLayoutInitialized = true;

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, dockSize);

        ImGuiID dockCenter = dockspaceId;
        const ImGuiID dockLeft = ImGui::DockBuilderSplitNode(
            dockCenter, ImGuiDir_Left, 0.18f, nullptr, &dockCenter);
        ImGuiID dockRight = ImGui::DockBuilderSplitNode(
            dockCenter, ImGuiDir_Right, 0.25f, nullptr, &dockCenter);
        ImGuiID dockBottom = ImGui::DockBuilderSplitNode(
            dockCenter, ImGuiDir_Down, 0.27f, nullptr, &dockCenter);
        const ImGuiID dockRightBottom = ImGui::DockBuilderSplitNode(
            dockRight, ImGuiDir_Down, 0.38f, nullptr, &dockRight);
        const ImGuiID dockBottomLeft = ImGui::DockBuilderSplitNode(
            dockBottom, ImGuiDir_Left, 0.46f, nullptr, &dockBottom);

        ImGui::DockBuilderDockWindow("Escena", dockLeft);
        ImGui::DockBuilderDockWindow("Propiedades", dockRight);
        ImGui::DockBuilderDockWindow("Iluminacion", dockRightBottom);
        ImGui::DockBuilderDockWindow("Datos de Render", dockRightBottom);
        ImGui::DockBuilderDockWindow("Recursos", dockBottomLeft);
        ImGui::DockBuilderDockWindow("Consola", dockBottom);
        ImGui::DockBuilderDockWindow("Render", dockBottom);
        ImGui::DockBuilderDockWindow("Rendimiento", dockBottom);
        ImGui::DockBuilderDockWindow("Vista 3D", dockCenter);
        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void GUI::drawEditorStatusBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = 24.0f;

    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - height),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height), ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.020f, 0.026f, 0.032f, 1.0f));

    if (ImGui::Begin("##WildvineStatusBar", nullptr, flags)) {
        ImGui::PushStyleColor(ImGuiCol_Text, kBrand);
        ImGui::TextUnformatted(m_statusMessage.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("|  Objetos: %u", m_cachedActorCount);
        ImGui::SameLine();
        ImGui::TextDisabled("|  Seleccion: %s",
            selectedActorIndex >= 0 ? "1" : "0");

        const char* rightText = "";
        char buffer[256] = {};
        sprintf_s(buffer, sizeof(buffer),
            "Viewport %.0fx%.0f   |   %.0f FPS   |   %u draw calls",
            m_viewportSize.x, m_viewportSize.y,
            m_cachedFps, m_cachedDrawCalls);
        rightText = buffer;
        const float textWidth = ImGui::CalcTextSize(rightText).x;
        const float targetX = ImGui::GetWindowWidth() - textWidth - 10.0f;
        if (targetX > ImGui::GetCursorPosX()) ImGui::SameLine(targetX);
        ImGui::TextDisabled("%s", rightText);
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void GUI::drawGBufferDebugPanel(ID3D11ShaderResourceView* albedoMetallicSRV,
	ID3D11ShaderResourceView* normalRoughnessSRV,
	ID3D11ShaderResourceView* worldAoSRV,
	ID3D11ShaderResourceView* emissiveAlphaSRV) {
	if (!m_showGBufferPanel) return;
	ImGui::Begin("Datos de Render", &m_showGBufferPanel);
	const char* modes[] = { "Iluminacion final", "Factor de sombra", "Albedo", "Normal global", "Posicion global", "Metal / Rugosidad / AO", "Emissive" };
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.92f, 0.70f, 1.0f));
	ImGui::TextUnformatted("Canal de visualizacion");
	ImGui::PopStyleColor();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::Combo("##DeferredDebugMode", &m_deferredDebugViewMode, modes, IM_ARRAYSIZE(modes));
	ImGui::Checkbox("Visualizar factor de sombra", &m_visualizeDeferredShadowFactor);
	ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

	auto placeholder = [&](ImVec2 size, const char* msg) {
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(14, 19, 21, 255), 4.0f);
		dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(36, 185, 137, 125), 4.0f);
		dl->AddText(ImVec2(p.x + 8.0f, p.y + 8.0f), IM_COL32(194, 226, 216, 255), msg);
		ImGui::Dummy(size);
		};
	float fullW = ImGui::GetContentRegionAvail().x;
	float cellW = (fullW - 8.0f) * 0.5f;
	ImVec2 cell(cellW, cellW * 0.5625f);
	auto target = [&](const char* label, ID3D11ShaderResourceView* srv) {
		ImGui::BeginGroup();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.64f, 0.88f, 0.80f, 1.0f));
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
	if (!m_showRenderPanel) return;
	ImGui::Begin("Render", &m_showRenderPanel);
	auto placeholder = [&](ImVec2 size, const char* msg) {
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(14, 19, 21, 255), 4.0f);
		dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), IM_COL32(36, 185, 137, 125), 4.0f);
		dl->AddText(ImVec2(p.x + 10.0f, p.y + 10.0f), IM_COL32(194, 226, 216, 255), msg);
		ImGui::Dummy(size);
		};
	auto section = [&](const char* label, ID3D11ShaderResourceView* srv, ImVec2 size) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.92f, 0.70f, 1.0f));
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
	if (!m_showLightingPanel) return;
	ImGui::Begin("Iluminacion", &m_showLightingPanel);
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
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.92f, 0.70f, 1.0f));
	ImGui::TextUnformatted("Luz direccional principal");
	ImGui::PopStyleColor();
	ImGui::Spacing();
	if (lightDir)   vec3Control("Direccion", lightDir, 0.0f, 90.0f);
	if (lightColor) vec3Control("Color", lightColor, 1.0f, 90.0f);
	ImGui::End();
}

void GUI::drawStatsPanel(
    float deltaTime,
    unsigned int drawCalls,
    const PerformanceStats& cullingStats) {

    static float frameHistory[120] = {};
    static float cullingHistory[120] = {};
    static int historyIndex = 0;
    static int historySamples = 0;
    static float accum = 0.0f;
    static int frames = 0;
    static float fps = 0.0f;
    static float averageFrameMs = 0.0f;

    const float dtMs = deltaTime * 1000.0f;
    frameHistory[historyIndex] = dtMs;
    cullingHistory[historyIndex] = cullingStats.cullingTimeMs;
    historyIndex = (historyIndex + 1) % IM_ARRAYSIZE(frameHistory);
    historySamples = (std::min)(
        historySamples + 1,
        IM_ARRAYSIZE(frameHistory));

    accum += deltaTime;
    ++frames;

    if (accum >= 0.25f) {
        fps = frames / accum;
        averageFrameMs = (frames > 0)
            ? (accum / frames) * 1000.0f
            : 0.0f;
        accum = 0.0f;
        frames = 0;
    }

    float minFrameMs = 0.0f;
    float maxFrameMs = 0.0f;
    float historyAverageMs = 0.0f;
    if (historySamples > 0) {
        minFrameMs = frameHistory[0];
        maxFrameMs = frameHistory[0];
        float total = 0.0f;
        for (int sampleIndex = 0;
            sampleIndex < historySamples;
            ++sampleIndex) {
            const float value = frameHistory[sampleIndex];
            minFrameMs = (std::min)(minFrameMs, value);
            maxFrameMs = (std::max)(maxFrameMs, value);
            total += value;
        }
        historyAverageMs = total /
            static_cast<float>(historySamples);
    }

    m_cachedFps = fps;
    m_cachedFrameMs = averageFrameMs;
    m_cachedDrawCalls = drawCalls;

    if (!m_showPerformance) {
        return;
    }

    ImGui::Begin("Rendimiento", &m_showPerformance);

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.30f, 0.92f, 0.70f, 1.0f));
    ImGui::SetWindowFontScale(1.7f);
    ImGui::Text("%.0f FPS", fps);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::TextDisabled("  %.2f ms", averageFrameMs);

    ImGui::TextDisabled(
        "Frame (120): min %.2f  prom %.2f  max %.2f ms",
        minFrameMs,
        historyAverageMs,
        maxFrameMs);

    ImGui::Spacing();
    ImGui::PlotLines(
        "##frametimes",
        frameHistory,
        historySamples,
        historySamples == IM_ARRAYSIZE(frameHistory)
            ? historyIndex
            : 0,
        "Frame time (ms)",
        0.0f,
        33.3f,
        ImVec2(
            ImGui::GetContentRegionAvail().x,
            70.0f));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.64f, 0.88f, 0.80f, 1.0f));
    ImGui::TextUnformatted("Render");
    ImGui::PopStyleColor();

    ImGui::Text("Draw calls reales: %u", drawCalls);
    ImGui::Text(
        "Submallas visibles: %u / %u",
        cullingStats.visibleSubmeshes,
        cullingStats.totalSubmeshes);
    ImGui::Text(
        "Triangulos visibles: %llu / %llu",
        cullingStats.visibleTriangles,
        cullingStats.totalTriangles);
    ImGui::TextDisabled(
        "Viewport: %.0f x %.0f",
        m_viewportSize.x,
        m_viewportSize.y);
    ImGui::TextDisabled(
        "Los draw calls incluyen pases de sombras y render adicional.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.64f, 0.88f, 0.80f, 1.0f));
    ImGui::TextUnformatted("Frustum Culling");
    ImGui::PopStyleColor();

    if (ImGui::Checkbox(
            "Activar Frustum Culling",
            &m_frustumCullingEnabled)) {

        m_statusMessage = m_frustumCullingEnabled
            ? "Frustum Culling activado"
            : "Frustum Culling desactivado";
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Descarta objetos cuya AABB queda completamente fuera del volumen visible de la camara.");
    }

    ImGui::Spacing();
    ImGui::Text(
        "Objetos renderizables: %u",
        cullingStats.totalRenderableObjects);

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.35f, 0.92f, 0.55f, 1.0f));
    ImGui::Text(
        "Visibles: %u",
        cullingStats.visibleObjects);
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.95f, 0.45f, 0.40f, 1.0f));
    ImGui::Text(
        "Descartados: %u",
        cullingStats.culledObjects);
    ImGui::PopStyleColor();

    ImGui::Text(
        "Tiempo del pase: %.3f ms",
        cullingStats.cullingTimeMs);

    const float culledPercentage =
        cullingStats.getCullPercentage();
    const float triangleCullPercentage =
        cullingStats.getTriangleCullPercentage();

    ImGui::Text(
        "Objetos descartados: %.1f%%",
        culledPercentage);
    ImGui::Text(
        "Triangulos evitados: %llu (%.1f%%)",
        cullingStats.culledTriangles,
        triangleCullPercentage);

    const float progress =
        (std::max)(0.0f,
            (std::min)(
                1.0f,
                culledPercentage / 100.0f));

    char progressLabel[64]{};
    sprintf_s(
        progressLabel,
        "%.1f%% objetos descartados",
        culledPercentage);

    ImGui::ProgressBar(
        progress,
        ImVec2(-1.0f, 0.0f),
        progressLabel);

    if (cullingStats.objectsWithoutBounds > 0) {
        ImGui::Spacing();
        ImGui::TextDisabled(
            "Sin AABB: %u (se mantienen visibles por seguridad)",
            cullingStats.objectsWithoutBounds);
    }

    if (!m_frustumCullingEnabled) {
        ImGui::Spacing();
        ImGui::TextDisabled(
            "Culling desactivado: todos los objetos renderizables se envian al renderer.");
    }

    ImGui::Spacing();
    ImGui::PlotLines(
        "##cullingtimes",
        cullingHistory,
        historySamples,
        historySamples == IM_ARRAYSIZE(cullingHistory)
            ? historyIndex
            : 0,
        "Culling time (ms)",
        0.0f,
        2.0f,
        ImVec2(
            ImGui::GetContentRegionAvail().x,
            55.0f));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.64f, 0.88f, 0.80f, 1.0f));
    ImGui::TextUnformatted("Octree espacial");
    ImGui::PopStyleColor();

    if (ImGui::Checkbox(
            "Usar Octree con Frustum",
            &m_octreeEnabled)) {
        m_statusMessage = m_octreeEnabled
            ? "Octree activado"
            : "Octree desactivado";
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Agrupa AABB en regiones. Los nodos completamente fuera se descartan de una sola vez.");
    }

    if (!m_frustumCullingEnabled && m_octreeEnabled) {
        ImGui::TextDisabled(
            "El Octree queda en espera mientras Frustum Culling este apagado.");
    }

    ImGui::SliderInt(
        "Profundidad maxima",
        &m_octreeMaxDepth,
        1,
        8);
    ImGui::SliderInt(
        "Objetos por nodo",
        &m_octreeCapacity,
        1,
        32);

    if (cullingStats.octreeEnabled) {
        ImGui::Spacing();
        ImGui::Text(
            "Entradas indexadas: %u",
            cullingStats.octreeEntries);
        ImGui::Text(
            "Nodos: %u | Hojas: %u",
            cullingStats.octreeTotalNodes,
            cullingStats.octreeLeafNodes);
        ImGui::Text(
            "Nodos probados: %u",
            cullingStats.octreeTestedNodes);
        ImGui::Text(
            "Nodos descartados completos: %u",
            cullingStats.octreeCulledNodes);
        ImGui::Text(
            "Nodos aceptados completos: %u",
            cullingStats.octreeAcceptedNodes);
        ImGui::Text(
            "Pruebas AABB individuales: %u / %u",
            cullingStats.octreeObjectTests,
            cullingStats.octreeEntries);

        const unsigned int avoidedTests =
            cullingStats.octreeEntries > cullingStats.octreeObjectTests
                ? cullingStats.octreeEntries - cullingStats.octreeObjectTests
                : 0u;
        ImGui::Text(
            "Pruebas individuales evitadas: %u",
            avoidedTests);

        ImGui::Text(
            "Construccion: %.3f ms | Consulta: %.3f ms",
            cullingStats.octreeBuildTimeMs,
            cullingStats.octreeQueryTimeMs);
        ImGui::TextDisabled(
            "Octree reconstruido este frame: %s",
            cullingStats.octreeRebuiltThisFrame ? "Si" : "No");
    }
    else {
        ImGui::TextDisabled(
            "Modo actual: Frustum Culling directo (objeto por objeto).");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.64f, 0.88f, 0.80f, 1.0f));
    ImGui::TextUnformatted("Particulas");
    ImGui::PopStyleColor();

    ImGui::Text("Emisores visibles: %u / %u",
        cullingStats.visibleParticleEmitters,
        cullingStats.particleEmitters);
    ImGui::Text("Particulas activas: %u / %u",
        cullingStats.activeParticles,
        cullingStats.particleCapacity);
    ImGui::Text("Spawn este frame: %u",
        cullingStats.particlesSpawnedThisFrame);
    ImGui::Text("CPU simulacion: %.3f ms",
        cullingStats.particleSimulationTimeMs);
    ImGui::Text("CPU billboards: %.3f ms",
        cullingStats.particleBillboardTimeMs);
    ImGui::TextDisabled(
        "Cada emisor activo se compacta en un solo draw call transparente.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        ImVec4(0.64f, 0.88f, 0.80f, 1.0f));
    ImGui::TextUnformatted("Debug visual");
    ImGui::PopStyleColor();

    ImGui::Checkbox(
        "Mostrar AABB de culling",
        &m_showCullingBounds);

    if (m_showCullingBounds) {
        ImGui::Indent();
        ImGui::Checkbox(
            "Incluir descartados",
            &m_showCulledBounds);
        ImGui::TextDisabled(
            "Verde = dentro | Naranja = intersecta | Rojo = fuera");
        ImGui::Unindent();
    }

    ImGui::Checkbox(
        "Mostrar volumen del Frustum",
        &m_showFrustumWireframe);

    ImGui::Checkbox(
        "Mostrar Octree",
        &m_showOctreeDebug);

    if (m_showOctreeDebug) {
        ImGui::Indent();
        ImGui::SliderInt(
            "Profundidad debug Octree",
            &m_octreeDebugDepth,
            0,
            m_octreeMaxDepth);
        ImGui::TextDisabled(
            "Azul = intersecta | Cian = dentro | Rojo = nodo descartado");
        if (!m_octreeEnabled) {
            ImGui::TextDisabled(
                "Activa 'Usar Octree con Frustum' para generar las regiones.");
        }
        ImGui::Unindent();
    }

    if (m_showCullingBounds || m_showFrustumWireframe) {
        ImGui::Checkbox(
            "Congelar Frustum de debug",
            &m_freezeFrustumDebug);

        if (m_freezeFrustumDebug) {
            ImGui::TextDisabled(
                "El volumen de debug esta congelado. El culling real sigue usando la camara actual.");
        }
        else if (m_showFrustumWireframe) {
            ImGui::TextDisabled(
                "Tip: congelalo y mueve la camara para observar el volumen desde fuera.");
        }
    }

    ImGui::End();
}

void GUI::drawConsolePanel() {
	if (!m_showConsole) return;
	ImGui::Begin("Consola", &m_showConsole);
	if (ImGui::Button("Limpiar")) Logger::get().clear();
	ImGui::SameLine();
	ImGui::Checkbox("Info", &m_logShowInfo); ImGui::SameLine();
	ImGui::Checkbox("Avisos", &m_logShowWarning); ImGui::SameLine();
	ImGui::Checkbox("Errores", &m_logShowError); ImGui::SameLine();
	ImGui::Checkbox("Auto scroll", &m_logAutoScroll); ImGui::SameLine();
	m_logFilter.Draw("Filtrar", 160.0f);
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
	if (ImGui::Begin("Vista de textura", &m_showPreview)) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.92f, 0.70f, 1.0f));
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
    if (!m_showContentBrowser) return;
    ImGui::Begin("Recursos", &m_showContentBrowser);

    ImGui::PushStyleColor(ImGuiCol_Text, kBrand);
    ImGui::TextUnformatted("CONTENT BROWSER");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextDisabled("Assets");

    ImGui::SetNextItemWidth(-125.0f);
    m_assetFilter.Draw("Buscar##AssetSearch");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(115.0f);
    ImGui::SliderFloat("##ThumbSize", &m_assetThumbnailSize, 64.0f, 128.0f, "Miniatura %.0f");
    ImGui::Separator();

    if (ImGui::BeginTabBar("##ContentTabs")) {
        const ImGuiTabItemFlags modelFlags =
            (m_requestedContentTab == 0) ? ImGuiTabItemFlags_SetSelected : 0;
        const ImGuiTabItemFlags textureFlags =
            (m_requestedContentTab == 1) ? ImGuiTabItemFlags_SetSelected : 0;

        if (ImGui::BeginTabItem("Modelos", nullptr, modelFlags)) {
            m_requestedContentTab = -1;
            std::vector<std::string> models;
            WIN32_FIND_DATAA fileData;
            HANDLE handle = FindFirstFileA("Assets\\Models\\*", &fileData);
            if (handle != INVALID_HANDLE_VALUE) {
                do {
                    if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                    std::string name = fileData.cFileName;
                    std::string lower = name;
                    for (char& character : lower) {
                        if (character >= 'A' && character <= 'Z') character = static_cast<char>(character + 32);
                    }
                    const bool isFbx = lower.size() >= 4 && lower.compare(lower.size() - 4, 4, ".fbx") == 0;
                    const bool isObj = lower.size() >= 4 && lower.compare(lower.size() - 4, 4, ".obj") == 0;
                    if ((isFbx || isObj) && m_assetFilter.PassFilter(name.c_str())) {
                        models.push_back(name);
                    }
                } while (FindNextFileA(handle, &fileData));
                FindClose(handle);
            }
            std::sort(models.begin(), models.end());

            ImGui::TextDisabled("Assets / Models");
            ImGui::SameLine();
            ImGui::TextDisabled("  %u elementos", static_cast<unsigned int>(models.size()));
            ImGui::Spacing();

            if (models.empty()) {
                ImGui::BeginChild("##EmptyModels", ImVec2(0.0f, 110.0f), true);
                ImGui::TextDisabled("No se encontraron modelos FBX u OBJ.");
                ImGui::TextDisabled("Colocalos en Assets/Models o cambia el filtro.");
                ImGui::EndChild();
            }

            const float cell = m_assetThumbnailSize;
            const float availableWidth = ImGui::GetContentRegionAvail().x;
            int perRow = static_cast<int>(availableWidth / (cell + 14.0f));
            if (perRow < 1) perRow = 1;
            int column = 0;

            for (const std::string& modelName : models) {
                ImGui::PushID(modelName.c_str());
                ImGui::BeginGroup();

                const bool selected = m_selectedAssetName == modelName;
                if (selected) ImGui::PushStyleColor(ImGuiCol_Button, kAccent);
                const std::string lowerName = [&]() {
                    std::string value = modelName;
                    for (char& character : value) {
                        if (character >= 'A' && character <= 'Z') character = static_cast<char>(character + 32);
                    }
                    return value;
                }();
                const char* typeLabel =
                    (lowerName.size() >= 4 && lowerName.compare(lowerName.size() - 4, 4, ".obj") == 0)
                    ? "OBJ"
                    : "FBX";
                const bool pressed = ImGui::Button(typeLabel, ImVec2(cell, cell));
                if (selected) ImGui::PopStyleColor();

                const std::string assetPath = "Assets/Models/" + modelName;
                if (pressed) m_selectedAssetName = modelName;
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s\nDoble clic para colocar al centro\nArrastra para elegir la posicion", modelName.c_str());
                }
                if (ImGui::IsItemHovered() &&
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    m_assetSpawnPath = assetPath;
                    m_assetSpawnScreenPosition = ImVec2(
                        m_viewportPos.x + m_viewportSize.x * 0.5f,
                        m_viewportPos.y + m_viewportSize.y * 0.5f);
                    m_assetSpawnRequested = true;
                    m_statusMessage = "Importando " + modelName;
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload(
                        "WILDVINE_MODEL_ASSET",
                        assetPath.c_str(),
                        assetPath.size() + 1);
                    ImGui::TextUnformatted("Colocar modelo");
                    ImGui::TextDisabled("%s", modelName.c_str());
                    ImGui::EndDragDropSource();
                }

                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cell);
                ImGui::TextWrapped("%s", modelName.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndGroup();
                ImGui::PopID();

                ++column;
                if (column < perRow) ImGui::SameLine();
                else column = 0;
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Texturas", nullptr, textureFlags)) {
            m_requestedContentTab = -1;
            unsigned int visibleCount = 0;
            for (const AssetThumb& texture : textureThumbs) {
                if (m_assetFilter.PassFilter(texture.name.c_str())) ++visibleCount;
            }

            ImGui::TextDisabled("Assets / Textures");
            ImGui::SameLine();
            ImGui::TextDisabled("  %u elementos", visibleCount);
            ImGui::Spacing();

            if (visibleCount == 0) {
                ImGui::BeginChild("##EmptyTextures", ImVec2(0.0f, 110.0f), true);
                ImGui::TextDisabled("No hay texturas que coincidan con el filtro.");
                ImGui::EndChild();
            }

            const float cell = m_assetThumbnailSize;
            const float availableWidth = ImGui::GetContentRegionAvail().x;
            int perRow = static_cast<int>(availableWidth / (cell + 14.0f));
            if (perRow < 1) perRow = 1;
            int column = 0;

            for (const AssetThumb& texture : textureThumbs) {
                if (!m_assetFilter.PassFilter(texture.name.c_str())) continue;
                ImGui::PushID(texture.name.c_str());
                ImGui::BeginGroup();
                if (texture.srv) ImGui::Image((ImTextureID)texture.srv, ImVec2(cell, cell));
                else ImGui::Dummy(ImVec2(cell, cell));

                if (ImGui::IsItemClicked()) m_selectedAssetName = texture.name;
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s\nDoble clic para ampliar", texture.name.c_str());
                }
                if (ImGui::IsItemHovered() &&
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    m_previewSRV = texture.srv;
                    m_previewLabel = texture.name;
                    m_showPreview = true;
                }

                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cell);
                ImGui::TextWrapped("%s", texture.name.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndGroup();
                ImGui::PopID();

                ++column;
                if (column < perRow) ImGui::SameLine();
                else column = 0;
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

	const ImVec2 clipMin = m_viewportPos;
	const ImVec2 clipMax(
		m_viewportPos.x + m_viewportSize.x,
		m_viewportPos.y + m_viewportSize.y);
	m_viewportDrawList->PushClipRect(clipMin, clipMax, true);

	// Halo (linea gruesa oscura) + linea de acento encima = se ve mas pro.
	// El clip evita que la caja atraviese otros paneles de la interfaz.
	for (int e = 0; e < 12; ++e) {
		int a = edges[e][0], b = edges[e][1];
		if (valid[a] && valid[b]) {
			m_viewportDrawList->AddLine(pts[a], pts[b], IM_COL32(0, 0, 0, 160), 4.0f);
			m_viewportDrawList->AddLine(pts[a], pts[b], IM_COL32(46, 230, 169, 240), 2.0f);
		}
	}

	m_viewportDrawList->PopClipRect();
}


void GUI::drawLightGizmos(
    const std::vector<EU::TSharedPointer<Actor>>& actors,
    Camera& camera) {
    if (!m_viewportDrawList ||
        m_viewportSize.x < 16.0f ||
        m_viewportSize.y < 16.0f) {
        return;
    }

    const ImVec2 clipMin = m_viewportPos;
    const ImVec2 clipMax(
        m_viewportPos.x + m_viewportSize.x,
        m_viewportPos.y + m_viewportSize.y);
    m_viewportDrawList->PushClipRect(clipMin, clipMax, true);

    const XMMATRIX viewProjection = camera.getView() * camera.getProj();

    auto worldToScreen = [&](const EU::Vector3& point, ImVec2& output) -> bool {
        const XMVECTOR clip = XMVector4Transform(
            XMVectorSet(point.x, point.y, point.z, 1.0f),
            viewProjection);
        const float w = XMVectorGetW(clip);
        if (w <= 0.0001f) {
            return false;
        }

        const float ndcX = XMVectorGetX(clip) / w;
        const float ndcY = XMVectorGetY(clip) / w;
        output.x = m_viewportPos.x +
            (ndcX * 0.5f + 0.5f) * m_viewportSize.x;
        output.y = m_viewportPos.y +
            (1.0f - (ndcY * 0.5f + 0.5f)) * m_viewportSize.y;
        return true;
    };

    for (const auto& actor : actors) {
        if (!actor || !actor->isActive()) {
            continue;
        }

        auto lightComponent = actor->getComponent<LightComponent>();
        auto transform = actor->getComponent<Transform>();
        if (!lightComponent || !transform || !lightComponent->isEnabled()) {
            continue;
        }

        lightComponent->syncWithTransform(*transform);
        const LightData& light = lightComponent->getLightData();
        const EU::Vector3 position = light.position;
        EU::Vector3 direction = light.direction.normalize();
        if (direction.isNearlyZero()) {
            direction = EU::Vector3(0.0f, -1.0f, 0.0f);
        }

        const float red = (std::max)(0.0f, (std::min)(light.color.x, 1.0f));
        const float green = (std::max)(0.0f, (std::min)(light.color.y, 1.0f));
        const float blue = (std::max)(0.0f, (std::min)(light.color.z, 1.0f));
        const ImU32 lightColor = IM_COL32(
            static_cast<int>(red * 255.0f),
            static_cast<int>(green * 255.0f),
            static_cast<int>(blue * 255.0f),
            235);

        ImVec2 positionScreen;
        if (!worldToScreen(position, positionScreen)) {
            continue;
        }

        m_viewportDrawList->AddCircleFilled(
            positionScreen,
            5.0f,
            lightColor,
            16);
        m_viewportDrawList->AddCircle(
            positionScreen,
            8.0f,
            IM_COL32(15, 20, 22, 230),
            16,
            2.0f);

        if (light.type == LightType::Point) {
            const float arm = 8.0f;
            m_viewportDrawList->AddLine(
                ImVec2(positionScreen.x - arm, positionScreen.y),
                ImVec2(positionScreen.x + arm, positionScreen.y),
                lightColor,
                2.0f);
            m_viewportDrawList->AddLine(
                ImVec2(positionScreen.x, positionScreen.y - arm),
                ImVec2(positionScreen.x, positionScreen.y + arm),
                lightColor,
                2.0f);
            continue;
        }

        const float visualLength = light.type == LightType::Directional
            ? 2.0f
            : (std::max)(0.25f, light.range);
        const EU::Vector3 endPosition = position + direction * visualLength;
        ImVec2 endScreen;
        if (!worldToScreen(endPosition, endScreen)) {
            continue;
        }

        m_viewportDrawList->AddLine(
            positionScreen,
            endScreen,
            lightColor,
            2.0f);

        if (light.type == LightType::Directional) {
            m_viewportDrawList->AddCircleFilled(endScreen, 3.0f, lightColor, 12);
            continue;
        }

        EU::Vector3 referenceUp(0.0f, 1.0f, 0.0f);
        if (EU::abs(EU::Vector3::dot(referenceUp, direction)) > 0.95f) {
            referenceUp = EU::Vector3(1.0f, 0.0f, 0.0f);
        }

        EU::Vector3 right = EU::Vector3::cross(referenceUp, direction).normalize();
        EU::Vector3 up = EU::Vector3::cross(direction, right).normalize();
        const float radians = light.spotAngle * EU::PI / 180.0f;
        const float coneRadius = std::tan(radians) * visualLength;
        const int segmentCount = 16;

        ImVec2 firstPoint;
        ImVec2 previousPoint;
        bool firstValid = false;
        bool previousValid = false;

        for (int segment = 0; segment <= segmentCount; ++segment) {
            const float angle =
                (static_cast<float>(segment) / static_cast<float>(segmentCount)) *
                EU::PI * 2.0f;
            const EU::Vector3 circleOffset =
                right * (std::cos(angle) * coneRadius) +
                up * (std::sin(angle) * coneRadius);
            const EU::Vector3 conePoint = endPosition + circleOffset;

            ImVec2 conePointScreen;
            const bool currentValid = worldToScreen(conePoint, conePointScreen);
            if (segment == 0) {
                firstPoint = conePointScreen;
                firstValid = currentValid;
            }

            if (currentValid && previousValid) {
                m_viewportDrawList->AddLine(
                    previousPoint,
                    conePointScreen,
                    lightColor,
                    1.5f);
            }

            if (currentValid &&
                (segment == 0 ||
                 segment == segmentCount / 4 ||
                 segment == segmentCount / 2 ||
                 segment == (segmentCount * 3) / 4)) {
                m_viewportDrawList->AddLine(
                    positionScreen,
                    conePointScreen,
                    lightColor,
                    1.0f);
            }

            previousPoint = conePointScreen;
            previousValid = currentValid;
        }

        if (firstValid && previousValid) {
            m_viewportDrawList->AddLine(
                previousPoint,
                firstPoint,
                lightColor,
                1.5f);
        }
    }

    m_viewportDrawList->PopClipRect();
}
