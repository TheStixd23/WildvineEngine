#pragma once
#include "Prerequisites.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <imgui_internal.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "ImGuizmo.h"

class Viewport;
class Window;
class Device;
class DeviceContext;
class Actor;
class Camera;


struct AssetThumb {
    std::string name;
    ID3D11ShaderResourceView* srv;
};
class GUI {
public:
    GUI() = default;
    ~GUI() = default;

    void awake();
    void init(Window& window, Device& device, DeviceContext& deviceContext);
    void update(Viewport& viewport, Window& window);
    void render();
    void destroy();

    void ToolBar();
    void closeApp();
    void toolTipData();

    void appleLiquidStyle(float opacity, ImVec4 accent);

    void vec3Control(const std::string& label, float* values, float resetValues = 0.0f, float columnWidth = 100.0f);
    void inspectorGeneral(EU::TSharedPointer<Actor> actor);
    void inspectorContainer(EU::TSharedPointer<Actor> actor);
    void outliner(const std::vector<EU::TSharedPointer<Actor>>& actors);
    void editTransform(Camera& cam, Window& window, EU::TSharedPointer<Actor> actor);

    void drawGizmoToolbar();
    void ToFloatArray(const XMMATRIX& mat, float* dest) {
        XMFLOAT4X4 temp;
        XMStoreFloat4x4(&temp, mat);
        memcpy(dest, &temp, sizeof(float) * 16);
    }

    void drawStudioTopRibbon();
    void drawViewportPanel(ID3D11ShaderResourceView* viewportSRV);
    void drawViewportGrid(Camera& cam);
    void drawEditorDockspace();

    void drawRenderDebugPanel(ID3D11ShaderResourceView* preShadowSRV,
        ID3D11ShaderResourceView* viewportSRV,
        ID3D11ShaderResourceView* shadowMapSRV);
    void drawGBufferDebugPanel(ID3D11ShaderResourceView* albedoMetallicSRV,
        ID3D11ShaderResourceView* normalRoughnessSRV,
        ID3D11ShaderResourceView* worldAoSRV,
        ID3D11ShaderResourceView* emissiveAlphaSRV);
    void drawLightingPanel(float* lightDir, float* lightColor);
   // void drawStatsPanel(float deltaTime);
    void drawStatsPanel(float deltaTime, unsigned int drawCalls);

    void drawContentBrowser(const std::vector<AssetThumb>& textureThumbs);


    void drawConsolePanel();
    void drawTexturePreview();

    bool consumeResetRequest() { bool r = m_resetRequested; m_resetRequested = false; return r; }
    bool consumeFocusRequest() { bool r = m_focusRequested; m_focusRequested = false; return r; }
    bool consumeFitRequest() { bool r = m_fitRequested;   m_fitRequested = false; return r; }


    bool consumeUndoRequest() { bool r = m_undoRequested; m_undoRequested = false; return r; }
    bool consumeRedoRequest() { bool r = m_redoRequested; m_redoRequested = false; return r; }


private:
    bool show_exit_popup = false;
    ImDrawList* m_viewportDrawList = nullptr;
    bool m_viewportActive = false;
    bool m_dockLayoutInitialized = false;

public:
    bool m_isUsingGizmo = false;
    int selectedActorIndex = -1;
    ImVec2 m_viewportPos = ImVec2(0.0f, 0.0f);
    ImVec2 m_viewportSize = ImVec2(0.0f, 0.0f);
    bool m_viewportHovered = false;
    bool m_viewportFocused = false;

    bool m_visualizeDeferredShadowFactor = false;
    int  m_deferredDebugViewMode = 0;

    bool m_logShowInfo = true;
    bool m_logShowWarning = true;
    bool m_logShowError = true;
    bool m_logAutoScroll = true;
    ImGuiTextFilter m_logFilter;

    bool m_resetRequested = false;

    ID3D11ShaderResourceView* m_previewSRV = nullptr;
    std::string m_previewLabel;
    bool m_showPreview = false;

    // Camara / grid / snap
    bool  m_showGrid = true;
    float m_gridSize = 10.0f;
    bool  m_snapEnabled = false;
    float m_snapTranslate = 0.5f;
    float m_snapRotate = 15.0f;
    float m_snapScale = 0.1f;
    bool  m_focusRequested = false;
    bool  m_fitRequested = false;

    bool m_undoRequested = false;
    bool m_redoRequested = false;

    bool m_duplicateRequested = false;
    bool m_deleteRequested = false;
    bool m_copyRequested = false;
    bool m_pasteRequested = false;
    bool m_savePrefabRequested = false;
    bool m_loadPrefabRequested = false;

    std::string m_assetSpawnPath;
    bool m_assetSpawnRequested = false;
    
    void drawSelectionOutline(Camera& cam, const EU::Vector3& localMin, const EU::Vector3& localMax, const XMMATRIX& world);




};
