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
    void drawEditorStatusBar();

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

    bool consumeCreateDirectionalLightRequest() {
        bool result = m_createDirectionalLightRequested;
        m_createDirectionalLightRequested = false;
        return result;
    }
    bool consumeCreatePointLightRequest() {
        bool result = m_createPointLightRequested;
        m_createPointLightRequested = false;
        return result;
    }
    bool consumeCreateSpotLightRequest() {
        bool result = m_createSpotLightRequested;
        m_createSpotLightRequested = false;
        return result;
    }
    bool consumeCreateStudioRigRequest() {
        bool result = m_createStudioRigRequested;
        m_createStudioRigRequested = false;
        return result;
    }
    bool consumeAimLightRequest() {
        bool result = m_aimLightRequested;
        m_aimLightRequested = false;
        return result;
    }

    bool consumeReparentRequest(int& childIndex, int& parentIndex) {
        if (!m_reparentRequested) {
            return false;
        }

        childIndex = m_reparentChildIndex;
        parentIndex = m_reparentParentIndex;
        m_reparentRequested = false;
        m_reparentChildIndex = -1;
        m_reparentParentIndex = -1;
        return true;
    }



    bool consumeNewSceneRequest() {
        bool result = m_newSceneRequested;
        m_newSceneRequested = false;
        return result;
    }
    bool consumeOpenSceneRequest() {
        bool result = m_openSceneRequested;
        m_openSceneRequested = false;
        return result;
    }
    bool consumeSaveSceneRequest() {
        bool result = m_saveSceneRequested;
        m_saveSceneRequested = false;
        return result;
    }
    bool consumeSaveSceneAsRequest() {
        bool result = m_saveSceneAsRequested;
        m_saveSceneAsRequested = false;
        return result;
    }
    bool consumeRecoverSceneRequest() {
        bool result = m_recoverSceneRequested;
        m_recoverSceneRequested = false;
        return result;
    }

    bool consumeUndoRequest() { bool r = m_undoRequested; m_undoRequested = false; return r; }
    bool consumeRedoRequest() { bool r = m_redoRequested; m_redoRequested = false; return r; }


private:
    bool show_exit_popup = false;
    ImDrawList* m_viewportDrawList = nullptr;
    bool m_viewportActive = false;
    bool m_dockLayoutInitialized = false;
    bool m_showUnsavedChangesPopup = false;
    int m_pendingSceneAction = 0;

    void requestSceneAction(int action);
    void dispatchPendingSceneAction();
    void drawUnsavedChangesPopup();


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

    bool m_newSceneRequested = false;
    bool m_openSceneRequested = false;
    bool m_saveSceneRequested = false;
    bool m_saveSceneAsRequested = false;
    bool m_recoverSceneRequested = false;
    bool m_recoveryAvailable = false;
    bool m_sceneDirty = false;
    std::string m_sceneDisplayName = "Sin titulo";

    bool m_undoRequested = false;
    bool m_redoRequested = false;

    bool m_duplicateRequested = false;
    bool m_deleteRequested = false;
    bool m_copyRequested = false;
    bool m_pasteRequested = false;
    bool m_savePrefabRequested = false;
    bool m_loadPrefabRequested = false;

    bool m_createDirectionalLightRequested = false;
    bool m_createPointLightRequested = false;
    bool m_createSpotLightRequested = false;
    bool m_createStudioRigRequested = false;
    bool m_aimLightRequested = false;

    // Solicitud generada por drag & drop en la jerarquia.
    // parentIndex == -1 significa mover el actor a la raiz de la escena.
    bool m_reparentRequested = false;
    int m_reparentChildIndex = -1;
    int m_reparentParentIndex = -1;

    std::string m_assetSpawnPath;
    ImVec2 m_assetSpawnScreenPosition = ImVec2(0.0f, 0.0f);
    bool m_assetSpawnRequested = false;

    // Estado visual del editor. Los paneles tecnicos permanecen ocultos
    // por defecto para dar prioridad al viewport y evitar una interfaz saturada.
    bool m_showOutliner = true;
    bool m_showInspector = true;
    bool m_showContentBrowser = true;
    bool m_showConsole = true;
    bool m_showPerformance = true;
    bool m_showLightingPanel = false;
    bool m_showRenderPanel = false;
    bool m_showGBufferPanel = false;
    bool m_showViewportOverlay = true;

    ImGuiTextFilter m_assetFilter;
    float m_assetThumbnailSize = 86.0f;
    int m_requestedContentTab = -1;
    std::string m_selectedAssetName;

    float m_cachedFps = 0.0f;
    float m_cachedFrameMs = 0.0f;
    unsigned int m_cachedDrawCalls = 0;
    unsigned int m_cachedActorCount = 0;
    std::string m_statusMessage = "Listo";
    
    void drawSelectionOutline(Camera& cam, const EU::Vector3& localMin, const EU::Vector3& localMax, const XMMATRIX& world);
    void drawLightGizmos(const std::vector<EU::TSharedPointer<Actor>>& actors, Camera& camera);




};
