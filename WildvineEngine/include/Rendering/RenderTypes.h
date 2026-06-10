/**
 * @file RenderTypes.h
 * @brief Declara la API de RenderTypes dentro del subsistema Rendering de WildvineEngine.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"

class Mesh;
class MaterialInstance;

/**
 * @enum MaterialDomain
 * @brief Define el comportamiento de visibilidad base de un material.
 */
enum class
	MaterialDomain {
	Opaque = 0,      ///< El material es completamente opaco.
	Masked,          ///< El material recorta píxeles basándose en un umbral de alfa (Alpha Cutoff).
	Transparent      ///< El material es translúcido y requiere ordenamiento por profundidad.
};

/**
 * @enum BlendMode
 * @brief Define la ecuación matemática utilizada para mezclar colores en el Framebuffer.
 */
enum class
	BlendMode {
	Opaque = 0,            ///< Sobrescribe el color de destino (sin mezcla).
	Alpha,                 ///< Mezcla estándar basada en la transparencia del origen (SrcAlpha, InvSrcAlpha).
	Additive,              ///< Suma el color de origen al color de destino (efectos de luz/fuego).
	PremultipliedAlpha     ///< Asume que el color RGB ya fue multiplicado por su canal Alfa.
};

/**
 * @enum RenderPassType
 * @brief Identifica la etapa actual dentro del ciclo del pipeline gráfico.
 */
enum class
	RenderPassType {
	Shadow = 0,      ///< Pase de generación del mapa de profundidad de sombras.
	Opaque,          ///< Pase de dibujado de geometría opaca.
	Skybox,          ///< Pase de dibujado del entorno/cielo.
	Transparent,     ///< Pase de dibujado de geometría con transparencia (ordenada atrás hacia adelante).
	Editor           ///< Pase específico para elementos de la interfaz o vistas del editor.
};

/**
 * @enum LightType
 * @brief Define el tipo de fuente de iluminación.
 */
enum class
	LightType {
	Directional = 0, ///< Luz infinita paralela (ej. el Sol).
	Point,           ///< Luz que emite en todas direcciones desde un punto específico.
	Spot             ///< Luz en forma de cono orientada hacia una dirección.
};

constexpr int kMaxLights = 8; ///< Límite máximo estándar de luces dinámicas, si aplica.

/**
 * @struct LightData
 * @brief Almacena las propiedades físicas y espaciales de una luz para la escena.
 */
struct
	LightData {
	LightType type = LightType::Directional; ///< Tipo de fuente de luz.
	EU::Vector3 color = EU::Vector3(1.0f, 1.0f, 1.0f); ///< Color base de la emisión.
	float intensity = 1.0f;                  ///< Multiplicador de fuerza luminosa.

	EU::Vector3 direction = EU::Vector3(0.0f, -1.0f, 0.0f); ///< Dirección de los rayos (Direccional/Spot).
	float range = 0.0f;                      ///< Distancia máxima de atenuación (Point/Spot).

	EU::Vector3 position = EU::Vector3(0.0f, 0.0f, 0.0f);   ///< Ubicación en el mundo (Point/Spot).
	float spotAngle = 0.0f;                  ///< Apertura del cono en grados o radianes (Spot).
};

/**
 * @struct MaterialParams
 * @brief Conjunto de propiedades de Renderizado Basado en Físicas (PBR) en CPU.
 */
struct
	MaterialParams {
	XMFLOAT4 baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); ///< Color base (RGBA).
	float metallic = 1.0f;           ///< Factor de metalicidad.
	float roughness = 1.0f;          ///< Factor de rugosidad superficial.
	float ao = 1.0f;                 ///< Oclusión ambiental base.
	float normalScale = 1.0f;        ///< Intensidad del mapa de normales.
	float emissiveStrength = 1.0f;   ///< Multiplicador de brillo propio.
	float alphaCutoff = 0.5f;        ///< Umbral para descartar píxeles en modo Masked.
};

/**
 * @struct CBPerFrame
 * @brief Buffer Constante (GPU) con datos globales que se actualizan una vez por frame.
 */
struct
	CBPerFrame {
	XMFLOAT4X4 View{};               ///< Matriz de vista de la cámara.
	XMFLOAT4X4 Projection{};         ///< Matriz de proyección de la cámara.
	XMFLOAT4X4 LightViewProjection{};///< Matriz combinada para la proyección de sombras direccionales.
	EU::Vector3 CameraPos{};         ///< Posición de la cámara en el mundo.
	float pad0 = 0.0f;               ///< Padding para alinear a 16 bytes.
	EU::Vector3 LightDir = EU::Vector3(0.0f, -1.0f, 0.0f); ///< Dirección de la luz principal de sombras.
	float pad1 = 0.0f;
	EU::Vector3 LightColor = EU::Vector3(1.0f, 1.0f, 1.0f);///< Color de la luz principal.
	float LightRange = 10.0f;        ///< Rango de la luz principal.
	EU::Vector3 LightPosition = EU::Vector3(0.0f, 3.0f, 0.0f); ///< Posición de la luz principal.
	int LightType = 0;               ///< Tipo de la luz principal.
	XMFLOAT4 LightPositionsRanges[kMaxSceneLights]{};      ///< Arreglo compactado (XYZ = Pos, W = Rango).
	XMFLOAT4 LightColorsTypes[kMaxSceneLights]{};          ///< Arreglo compactado (XYZ = Color, W = Tipo).
	XMFLOAT4 LightDirectionsIntensities[kMaxSceneLights]{};///< Arreglo compactado (XYZ = Dir, W = Intensidad).
	int LightCount = 0;              ///< Cantidad de luces activas en la escena actual.
	XMFLOAT3 pad2 = XMFLOAT3(0.0f, 0.0f, 0.0f);            ///< Padding para alineación en HLSL.
};

/**
 * @struct CBPerObject
 * @brief Buffer Constante (GPU) con datos de transformación por cada malla dibujada.
 */
struct
	CBPerObject {
	XMFLOAT4X4 World{};              ///< Matriz de transformación (World) del objeto en la escena.
};

/**
 * @struct CBPerMaterial
 * @brief Buffer Constante (GPU) que mapea los parámetros PBR numéricos al shader.
 */
struct
	CBPerMaterial {
	XMFLOAT4 BaseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	float Metallic = 1.0f;
	float Roughness = 1.0f;
	float AO = 1.0f;
	float NormalScale = 1.0f;
	float EmissiveStrength = 1.0f;
	float AlphaCutoff = 0.0f;
	float pad0 = 0.0f;               ///< Paddings obligatorios para respetar el tamaño de bloque en HLSL.
	float pad1 = 0.0f;
	float pad2 = 0.0f;
	float pad3 = 0.0f;
	float pad4 = 0.0f;
	float pad5 = 0.0f;
};

/**
 * @struct RenderObject
 * @brief Encapsula una entidad dibujable dentro del pipeline del motor gráfico.
 */
struct
	RenderObject {
	Mesh* mesh = nullptr;                               ///< Puntero a la malla geométrica.
	MaterialInstance* materialInstance = nullptr;       ///< Material principal a utilizar si hay solo un slot.
	std::vector<MaterialInstance*> materialInstances;   ///< Lista de materiales si la malla tiene múltiples submallas.
	XMMATRIX world = XMMatrixIdentity();                ///< Transformación espacial en el mundo.
	bool castShadow = true;                             ///< Bandera que indica si el objeto proyecta sombra.
	bool transparent = false;                           ///< Bandera que clasifica al objeto para la cola de transparencias.
	float distanceToCamera = 0.0f;                      ///< Distancia precalculada (para ordenamiento Z).
};