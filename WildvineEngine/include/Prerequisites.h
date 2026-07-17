#pragma once
// Librerias STD
#include <string>
#include <sstream>
#include <vector>

// Evita que Windows defina macros min/max que rompen std::min y std::max.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Proteccion adicional por si Windows.h fue incluido antes desde otro archivo.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <xnamath.h>
#include <thread>
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <array>
#include <algorithm> 
#include <cmath>
// Librerias DirectX
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include "Resource.h"
#include "resource.h"

// Third Party Libraries
#include "EngineUtilities/Vectors/Vector2.h"
#include "EngineUtilities/Vectors/Vector3.h"
#include "EngineUtilities\Memory\TSharedPointer.h"
#include "EngineUtilities\Memory\TWeakPointer.h"
#include "EngineUtilities\Memory\TStaticPtr.h"
#include "EngineUtilities\Memory\TUniquePtr.h"

// Logger del editor (consola)
#include "Logger.h"

// MACROS
#define SAFE_RELEASE(x) if(x != nullptr) x->Release(); x = nullptr;

#define MESSAGE( classObj, method, state )   \
{                                            \
   std::wostringstream os_;                  \
   os_ << classObj << "::" << method << " : " << "[CREATION OF RESOURCE " << ": " << state << "]"; \
   OutputDebugStringW( os_.str().c_str() );  \
   Logger::get().addW(LogLevel::Info, os_.str()); \
}

#define ERROR(classObj, method, errorMSG)                     \
{                                                             \
    try {                                                     \
        std::wostringstream os_;                              \
        os_ << L"ERROR : " << classObj << L"::" << method     \
            << L" : " << errorMSG << L"";                   \
        OutputDebugStringW(os_.str().c_str());                \
        Logger::get().addW(LogLevel::Error, os_.str());       \
    } catch (...) {                                           \
        OutputDebugStringW(L"Failed to log error message.");\
    }                                                         \
}

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    EU::Vector3 Position;
    EU::Vector3 Normal;
    EU::Vector3 Tangent;
    EU::Vector3 Bitangent;
    EU::Vector2 TextureCoordinate;
};

struct
    SkyboxVertex {
    float x, y, z;
};

struct CBNeverChanges
{
    XMMATRIX mView;
};

struct CBSkybox
{
    XMMATRIX mviewProj;
};

struct CBChangeOnResize
{
    XMMATRIX mProjection;
};

struct CBMain
{
    XMFLOAT4X4 View;
    XMFLOAT4X4 Projection;
    EU::Vector3 CameraPos;
    float pad0;
    EU::Vector3 LightDir;
    float pad1;
    EU::Vector3 LightColor;
    float pad2;
};

struct CBChangesEveryFrame
{
    XMMATRIX mWorld;
    XMFLOAT4 vMeshColor;
};

enum ExtensionType {
    DDS = 0,
    PNG = 1,
    JPG = 2,
    TGA = 3,
    BMP = 4
};

enum ShaderType {
    VERTEX_SHADER = 0,
    PIXEL_SHADER = 1
};

/**
 * @enum ComponentType
 * @brief Tipos de componentes disponibles en el juego.
 */
enum
    ComponentType {
    NONE = 0,
    TRANSFORM = 1,
    MESH = 2,
    MATERIAL = 3,
    HIERARCHY = 4,
    LIGHT = 5
};
