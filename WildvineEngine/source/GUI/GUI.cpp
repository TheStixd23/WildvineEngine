#include "EngineUtilities\GUI\GUI.h"
#include "Viewport.h"
#include "Window.h"
#include "Device.h"
#include "DeviceContext.h"
#include "MeshComponent.h"
#include "ECS\Actor.h"
#include "EngineUtilities\Utilities\Camera.h"

// ==============================================================================
// VARIABLES ESTÁTICAS GLOBALES
// ==============================================================================
static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);

// ==============================================================================
// CICLO DE VIDA DE LA GUI (Init, Update, Render, Destroy)
// ==============================================================================

void GUI::init(Window& window, Device& device, DeviceContext& deviceContext) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Usamos tu misma funcion, pero la hemos transformado en un estilo "Tech Green" por dentro
    appleLiquidStyle(1.0f, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));

    ImGui_ImplWin32_Init(window.m_hWnd);
    ImGui_ImplDX11_Init(device.m_device, deviceContext.m_deviceContext);

    toolTipData();
    selectedActorIndex = 0;
}

void GUI::update(Viewport& viewport, Window& window) {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::BeginFrame();
    ImGuiIO& io = ImGui::GetIO();
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
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

// ==============================================================================
// LAYOUT PRINCIPAL DEL EDITOR (Dockspace, Menús, Popups)
// ==============================================================================

void GUI::drawEditorDockspace() {
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();

    const float topOffset = 96.0f;

    ImVec2 dockPos = ImVec2(mainViewport->Pos.x, mainViewport->Pos.y + topOffset);
    ImVec2 dockSize = ImVec2(mainViewport->Size.x, mainViewport->Size.y - topOffset);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_MenuBar;

    ImGui::SetNextWindowPos(dockPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(dockSize, ImGuiCond_Always);
    ImGui::SetNextWindowViewport(mainViewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("##MainEditorDockspace", nullptr, window_flags);

    ImGuiID dockspace_id = ImGui::GetID("##EditorDockspace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void GUI::drawStudioTopRibbon() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    const float menuBarHeight = 24.0f;
    const float ribbonHeight = 72.0f;

    // 1) MENÚ SUPERIOR (Estilo verde oscuro)
    ImGui::SetNextWindowPos(viewport->Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, menuBarHeight), ImGuiCond_Always);

    ImGuiWindowFlags menuFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.08f, 0.02f, 1.0f)); // Verde muy oscuro

    if (ImGui::Begin("##StudioMenuBar", nullptr, menuFlags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("New Place");
                ImGui::MenuItem("Open Place");
                ImGui::MenuItem("Save");
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) show_exit_popup = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) { ImGui::MenuItem("Undo"); ImGui::MenuItem("Redo"); ImGui::EndMenu(); }
            if (ImGui::BeginMenu("View")) { ImGui::MenuItem("Explorer"); ImGui::MenuItem("Properties"); ImGui::EndMenu(); }
            if (ImGui::BeginMenu("Plugins")) { ImGui::MenuItem("Manage Plugins"); ImGui::EndMenu(); }
            if (ImGui::BeginMenu("Test")) { ImGui::MenuItem("Play"); ImGui::MenuItem("Stop"); ImGui::EndMenu(); }
            if (ImGui::BeginMenu("Window")) { ImGui::MenuItem("Reset Layout"); ImGui::EndMenu(); }
            if (ImGui::BeginMenu("Help")) { ImGui::MenuItem("Documentation"); ImGui::EndMenu(); }
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    // 2) RIBBON PRINCIPAL
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ribbonHeight), ImGuiCond_Always);

    ImGuiWindowFlags ribbonFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.12f, 0.04f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.06f, 0.18f, 0.06f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.3f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.15f, 1.0f));

    if (ImGui::Begin("##StudioRibbon", nullptr, ribbonFlags)) {
        auto ribbonButton = [&](const char* id, const char* topText, const char* bottomText, ImVec2 size, bool active = false) -> bool {
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));

            bool pressed = ImGui::Button(id, size);
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 topSize = ImGui::CalcTextSize(topText);
            ImVec2 bottomSize = ImGui::CalcTextSize(bottomText);
            float centerX = (min.x + max.x) * 0.5f;

            drawList->AddText(ImVec2(centerX - topSize.x * 0.5f, min.y + 10.0f), ImGui::GetColorU32(ImGuiCol_Text), topText);
            drawList->AddText(ImVec2(centerX - bottomSize.x * 0.5f, min.y + 34.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled), bottomText);

            if (active) ImGui::PopStyleColor();
            return pressed;
            };

        auto separatorGroup = [&]() {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(6.0f, 1.0f));
            ImGui::SameLine();
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddLine(ImVec2(p.x, p.y), ImVec2(p.x, p.y + 48.0f), IM_COL32(30, 90, 30, 255), 2.0f);
            ImGui::Dummy(ImVec2(8.0f, 48.0f));
            ImGui::SameLine();
            };

        const ImVec2 btnSize(72.0f, 52.0f);

        ribbonButton("##Select", "Select", "Cursor", btnSize, false); ImGui::SameLine();
        if (ribbonButton("##Move", "Move", "W", btnSize, mCurrentGizmoOperation == ImGuizmo::TRANSLATE)) mCurrentGizmoOperation = ImGuizmo::TRANSLATE; ImGui::SameLine();
        if (ribbonButton("##Scale", "Scale", "R", btnSize, mCurrentGizmoOperation == ImGuizmo::SCALE)) mCurrentGizmoOperation = ImGuizmo::SCALE; ImGui::SameLine();
        if (ribbonButton("##Rotate", "Rotate", "E", btnSize, mCurrentGizmoOperation == ImGuizmo::ROTATE)) mCurrentGizmoOperation = ImGuizmo::ROTATE; ImGui::SameLine();
        ribbonButton("##Transform", "Transform", "Tool", btnSize, false);

        separatorGroup();

        ribbonButton("##Part", "Part", "Mesh", btnSize, false); ImGui::SameLine();
        ribbonButton("##Terrain", "Terrain", "Edit", btnSize, false); ImGui::SameLine();
        ribbonButton("##Material", "Material", "Editor", btnSize, false); ImGui::SameLine();
        ribbonButton("##Color", "Color", "Picker", btnSize, false);

        separatorGroup();

        ribbonButton("##Explorer", "Explorer", "Panel", btnSize, false); ImGui::SameLine();
        ribbonButton("##Properties", "Properties", "Panel", btnSize, false); ImGui::SameLine();
        ribbonButton("##Toolbox", "Toolbox", "Assets", btnSize, false);
    }
    ImGui::End();

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);
}

void GUI::ToolBar() { /* Se mantiene igual, oculta a favor del ribbon superior en la logica base */ }

void GUI::closeApp() {
    if (show_exit_popup) {
        ImGui::OpenPopup("Exit?");
        show_exit_popup = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Exit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Estas a punto de salir de la aplicacion.\nEstas seguro?\n\n");
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) { exit(0); ImGui::CloseCurrentPopup(); }
        ImGui::SetItemDefaultFocus(); ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}

// ==============================================================================
// PANELES (Inspector, Outliner, Viewport)
// ==============================================================================

void GUI::inspectorGeneral(EU::TSharedPointer<Actor> actor) {
    ImGui::Begin("Inspector");

    bool isStatic = false;
    ImGui::Checkbox("##Static", &isStatic);
    ImGui::SameLine();

    char objectName[128] = "Cube";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.6f);
    ImGui::InputText("##ObjectName", &actor->getName()[0], IM_ARRAYSIZE(objectName));
    ImGui::SameLine();

    if (ImGui::Button("Icon")) {}

    ImGui::Separator();

    const char* tags[] = { "Untagged", "Player", "Enemy", "Environment" };
    static int currentTag = 0;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
    ImGui::Combo("Tag", &currentTag, tags, IM_ARRAYSIZE(tags));
    ImGui::SameLine();

    const char* layers[] = { "Default", "TransparentFX", "Ignore Raycast", "Water", "UI" };
    static int currentLayer = 0;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f);
    ImGui::Combo("Layer", &currentLayer, layers, IM_ARRAYSIZE(layers));

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        inspectorContainer(actor);
    }
    ImGui::End();
}

void GUI::inspectorContainer(EU::TSharedPointer<Actor> actor) {
    vec3Control("Position", const_cast<float*>(actor->getComponent<Transform>()->getPosition().data()));
    vec3Control("Rotation", const_cast<float*>(actor->getComponent<Transform>()->getRotation().data()));
    vec3Control("Scale", const_cast<float*>(actor->getComponent<Transform>()->getScale().data()));
}

void GUI::outliner(const std::vector<EU::TSharedPointer<Actor>>& actors) {
    ImGui::Begin("Hierarchy");

    static ImGuiTextFilter filter;
    filter.Draw("Search...", 180.0f);

    ImGui::Separator();

    for (int i = 0; i < actors.size(); ++i) {
        const auto& actor = actors[i];
        std::string actorName = actor ? actor->getName() : "Actor";

        if (!filter.PassFilter(actorName.c_str())) continue;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (selectedActorIndex == i) flags |= ImGuiTreeNodeFlags_Selected;

        bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s", actorName.c_str());

        if (ImGui::IsItemClicked()) selectedActorIndex = i;

        if (nodeOpen) {
            ImGui::Text("Position: %.2f, %.2f, %.2f",
                actor->getComponent<Transform>().get()->getPosition().x,
                actor->getComponent<Transform>().get()->getPosition().y,
                actor->getComponent<Transform>().get()->getPosition().z);
            ImGui::TreePop();
        }
    }
    ImGui::End();
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

        m_viewportPos = panelMin;
        m_viewportSize = panelSize;

        if (viewportSRV) {
            ImGui::Image((ImTextureID)viewportSRV, panelSize);
        }
        else {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 panelMax(panelMin.x + panelSize.x, panelMin.y + panelSize.y);

            drawList->AddRectFilled(panelMin, panelMax, IM_COL32(10, 30, 10, 255)); // Fondo Verde viewport vacio
            drawList->AddText(ImVec2(panelMin.x + 12.0f, panelMin.y + 12.0f), IM_COL32(100, 255, 100, 255), "Viewport sin textura");
        }

        m_viewportHovered = ImGui::IsItemHovered();
        m_viewportActive = ImGui::IsItemActive();
        m_viewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// ==============================================================================
// GIZMOS Y MANIPULACIÓN 3D
// ==============================================================================

void GUI::editTransform(Camera& cam, Window& window, EU::TSharedPointer<Actor> actor) {
    if (!actor) return;

    static ImGuizmo::MODE mCurrentGizmoMode = ImGuizmo::WORLD;
    auto transform = actor->getComponent<Transform>();
    if (!transform) return;

    float rectX = m_viewportPos.x;
    float rectY = m_viewportPos.y;
    float rectW = m_viewportSize.x;
    float rectH = m_viewportSize.y;

    if (rectW < 64.0f || rectH < 64.0f) {
        m_isUsingGizmo = false;
        return;
    }

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

    float snapValue = 25.0f;
    if (mCurrentGizmoOperation == ImGuizmo::ROTATE)    snapValue = 5.0f;
    if (mCurrentGizmoOperation == ImGuizmo::TRANSLATE) snapValue = 0.5f;

    float snap[3] = { snapValue, snapValue, snapValue };
    bool useSnap = ImGui::GetIO().KeyCtrl;

    bool canManipulate = m_viewportHovered || m_viewportActive || m_isUsingGizmo;

    if (canManipulate) {
        ImGuizmo::Manipulate(vArr, pArr, mCurrentGizmoOperation, mCurrentGizmoMode, mArr, nullptr, useSnap ? snap : nullptr);
    }

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
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("GizmoToolBar", nullptr, window_flags)) {
        auto buttonMode = [&](const char* label, ImGuizmo::OPERATION op, const char* shortcut) {
            bool isActive = (mCurrentGizmoOperation == op);
            if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f)); // Verde brillante al activarse

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
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// ==============================================================================
// UTILIDADES Y ESTILIZACIÓN DE LA INTERFAZ
// ==============================================================================

void GUI::vec3Control(const std::string& label, float* values, float resetValue, float columnWidth) {
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

    ImGui::PushID(label.c_str());
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text(label.c_str());
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    // X - Mantenemos rojo para que la UX no confunda al manipular 3D, pero más apagado para que no rompa el tema
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.5f, 0.1f, 0.1f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.8f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.5f, 0.1f, 0.1f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("X", buttonSize)) values[0] = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values[0], 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Y - Verde puro
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.6f, 0.1f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.8f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.6f, 0.1f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize)) values[1] = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // Z - Azul, igual apagado para respetar el tema pero manteniendo usabilidad XYZ
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.1f, 0.5f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.2f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.1f, 0.5f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z", buttonSize)) values[2] = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
}

void GUI::toolTipData() {}

void GUI::appleLiquidStyle(float opacity, ImVec4 accent) {
    // AUNQUE SE LLAME "appleLiquidStyle", AHORA ES UN ESTILO "TECH GREEN" POR DENTRO.
    // Esto evita que tengas que cambiar el nombre en tu archivo .h

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 1. GEOMETRÍA "SHARP/TERMINAL" (Cambio drástico: adiós redondeos)
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding = 0.0f;

    // 2. BORDES REMARCADOS
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;

    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);

    // 3. PALETA DE COLORES "MATRIX/SCI-FI"
    const ImVec4 txt = ImVec4(0.8f, 1.0f, 0.8f, 1.0f);     // Texto verde pálido
    const ImVec4 bgDark = ImVec4(0.04f, 0.08f, 0.04f, 1.0f); // Fondo principal
    const ImVec4 bgMid = ImVec4(0.06f, 0.12f, 0.06f, 1.0f);  // Paneles
    const ImVec4 border = ImVec4(0.1f, 0.4f, 0.1f, 1.0f);    // Bordes verde brillante

    colors[ImGuiCol_Text] = txt;
    colors[ImGuiCol_TextDisabled] = ImVec4(0.4f, 0.6f, 0.4f, 1.0f);
    colors[ImGuiCol_WindowBg] = bgDark;
    colors[ImGuiCol_ChildBg] = bgMid;
    colors[ImGuiCol_PopupBg] = bgDark;
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0.0f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.02f, 0.04f, 0.02f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.08f, 0.25f, 0.08f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.1f, 0.35f, 0.1f, 1.0f);

    colors[ImGuiCol_TitleBg] = bgMid;
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.20f, 0.08f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = bgDark;

    colors[ImGuiCol_MenuBarBg] = bgDark;

    colors[ImGuiCol_ScrollbarBg] = bgDark;
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1f, 0.3f, 0.1f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.15f, 0.45f, 0.15f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.2f, 0.6f, 0.2f, 1.0f);

    colors[ImGuiCol_CheckMark] = accent; // Verde neon (pasado por parametro arriba)
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);

    colors[ImGuiCol_Button] = ImVec4(0.06f, 0.16f, 0.06f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.1f, 0.3f, 0.1f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.45f, 0.15f, 1.0f);

    colors[ImGuiCol_Header] = ImVec4(0.08f, 0.20f, 0.08f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.35f, 0.12f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.45f, 0.15f, 1.0f);

    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.2f, 0.6f, 0.2f, 1.0f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);

    colors[ImGuiCol_Tab] = ImVec4(0.05f, 0.10f, 0.05f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.1f, 0.3f, 0.1f, 1.0f);
    colors[ImGuiCol_TabActive] = bgMid;
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.04f, 0.08f, 0.04f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = bgDark;

    colors[ImGuiCol_DockingPreview] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_DockingEmptyBg] = bgDark;

    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.06f, 0.15f, 0.06f, 1.0f);
    colors[ImGuiCol_TableBorderStrong] = border;
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.08f, 0.25f, 0.08f, 1.0f);
    colors[ImGuiCol_TableRowBg] = bgDark;
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.05f, 0.10f, 0.05f, 1.0f);

    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.1f, 0.4f, 0.1f, 0.5f);
    colors[ImGuiCol_NavHighlight] = accent;
}