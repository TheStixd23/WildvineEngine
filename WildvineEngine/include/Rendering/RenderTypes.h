/**
 * @file RenderTypes.h
 * @brief Declara la API de tipos de datos, estructuras y enumeraciones para el subsistema Rendering de MonacoEngine3.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"

class Mesh;
class MaterialInstance;

/**
 * @enum MaterialDomain
 * @brief Define el dominio geométrico y de visibilidad de un material.
 * * Permite al pipeline saber cómo debe procesar la geometría (ej. z-sorting para transparentes).
 */
enum class MaterialDomain {
	Opaque = 0,     ///< Geometría sólida sin recortes. Escribe en el Depth Buffer.
	Masked,         ///< Geometría opaca que puede descartar píxeles (Alpha Clip). Escribe en Depth.
	Transparent     ///< Geometría semi-transparente. Requiere ser ordenada y típicamente no escribe en Depth.
};

/**
 * @enum BlendMode
 * @brief Define la ecuación matemática de mezcla de colores en el rasterizador.
 */
enum class BlendMode {
	Opaque = 0,             ///< Reemplaza el color existente en el buffer (Sin mezcla).
	Alpha,                  ///< Interpola basado en el canal Alpha del origen (SrcAlpha, InvSrcAlpha).
	Additive,               ///< Suma los colores (útil para luces, fuego, destellos).
	PremultipliedAlpha      ///< Mezcla optimizada asumiendo que el color base ya fue multiplicado por su Alpha.
};

/**
 * @enum RenderPassType
 * @brief Identifica las diferentes etapas principales del pipeline de renderizado de MonacoEngine3.
 */
enum class RenderPassType {
	Shadow = 0,     ///< Pase exclusivo de profundidad para generación de mapas de sombras.
	Opaque,         ///< Pase de dibujo para geometría con materiales opacos o enmascarados.
	Skybox,         ///< Pase de dibujo del entorno/caja del cielo.
	Transparent,    ///< Pase de dibujo para geometría con blending activado.
	Editor          ///< Pase final o superpuesto utilizado por las herramientas gráficas del editor.
};

/**
 * @enum LightType
 * @brief Define la forma de propagación de una fuente de luz.
 */
enum class LightType {
	Directional = 0, ///< Luz infinita paralela (ej. el Sol). No usa posición, solo dirección.
	Point,           ///< Luz que emite en todas direcciones desde un punto finito.
	Spot             ///< Luz cónica que emite desde un punto en una dirección específica.
};

/**
 * @struct LightData
 * @brief Estructura de datos que agrupa las propiedades de una fuente de luz en la escena.
 */
struct LightData {
	LightType type = LightType::Directional;          ///< Tipo de fuente de luz.
	EU::Vector3 color = EU::Vector3(1.0f, 1.0f, 1.0f);///< Color de la emisión luminosa en formato RGB.
	float intensity = 1.0f;                           ///< Multiplicador de brillo de la luz.

	EU::Vector3 direction = EU::Vector3(0.0f, -1.0f, 0.0f); ///< Vector de dirección normalizado (usado en Directional y Spot).
	float range = 0.0f;                               ///< Radio máximo de atenuación (usado en Point y Spot).

	EU::Vector3 position = EU::Vector3(0.0f, 0.0f, 0.0f); ///< Posición en el espacio del mundo (usado en Point y Spot).
	float spotAngle = 0.0f;                           ///< Ángulo de apertura del cono de luz en radianes o grados (usado en Spot).
};

/**
 * @struct MaterialParams
 * @brief Parámetros escalares y vectoriales base para el modelo PBR de un material.
 */
struct MaterialParams {
	XMFLOAT4 baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); ///< Tinte general multiplicado por la textura Albedo.
	float metallic = 1.0f;           ///< Modificador del mapa de metalizado (0.0 dieléctrico a 1.0 metálico).
	float roughness = 1.0f;          ///< Modificador del mapa de rugosidad (0.0 liso a 1.0 rugoso).
	float ao = 1.0f;                 ///< Intensidad del oscurecimiento por Oclusión Ambiental.
	float normalScale = 1.0f;        ///< Escala de intensidad del mapa de normales.
	float emissiveStrength = 1.0f;   ///< Multiplicador para la cantidad de luz propia emitida.
	float alphaCutoff = 0.5f;        ///< Umbral para descartar píxeles en materiales Masked.
};

/**
 * @struct CBPerFrame
 * @brief Constant Buffer estructurado para datos globales que cambian solo una vez por frame.
 * @note Se incluyen campos `pad` explícitos para forzar el empaquetado de memoria en múltiplos de 16 bytes, requerido por HLSL.
 */
struct alignas(16) CBPerFrame {
	XMFLOAT4X4 View{};                    ///< Matriz de Vista de la cámara principal.
	XMFLOAT4X4 Projection{};              ///< Matriz de Proyección de la cámara principal.
	XMFLOAT4X4 LightViewProjection{};     ///< Matriz combinada Vista/Proyección desde la perspectiva de la luz (para sombras).
	EU::Vector3 CameraPos{};              ///< Posición global de la cámara (útil para cálculos especulares/fresnel).
	float pad0 = 0.0f;                    ///< Relleno para alineación HLSL.
	EU::Vector3 LightDir = EU::Vector3(0.0f, -1.0f, 0.0f); ///< Dirección de la luz principal.
	float pad1 = 0.0f;                    ///< Relleno para alineación HLSL.
	EU::Vector3 LightColor = EU::Vector3(1.0f, 1.0f, 1.0f);///< Color irradiado por la luz principal.
	float pad2 = 0.0f;                    ///< Relleno para alineación HLSL.
};

/**
 * @struct CBPerObject
 * @brief Constant Buffer estructurado para datos específicos transformacionales de un modelo.
 */
struct alignas(16) CBPerObject {
	XMFLOAT4X4 World{}; ///< Matriz de transformación (Mundo) de un objeto para pasarlo de Local a World space.
};

/**
 * @struct CBPerMaterial
 * @brief Constant Buffer estructurado que se envía a la GPU detallando las propiedades visuales de un material.
 * @note Incluye rellenos explícitos (`pad0`-`pad5`) para garantizar que la estructura respete reglas de alineación estricta de HLSL (16 bytes por registro).
 */
struct alignas(16) CBPerMaterial {
	XMFLOAT4 BaseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); ///< Tinte base (Albedo/Diffuse).
	float Metallic = 1.0f;           ///< Nivel metálico.
	float Roughness = 1.0f;          ///< Nivel de rugosidad.
	float AO = 1.0f;                 ///< Oclusión ambiental.
	float NormalScale = 1.0f;        ///< Intensidad del Normal map.
	float EmissiveStrength = 1.0f;   ///< Intensidad Emisiva.
	float AlphaCutoff = 0.0f;        ///< Umbral de recorte Alpha.
	float pad0 = 0.0f;               ///< Relleno para alineación HLSL.
	float pad1 = 0.0f;               ///< Relleno para alineación HLSL.
	float pad2 = 0.0f;               ///< Relleno para alineación HLSL.
	float pad3 = 0.0f;               ///< Relleno para alineación HLSL.
	float pad4 = 0.0f;               ///< Relleno para alineación HLSL.
	float pad5 = 0.0f;               ///< Relleno para alineación HLSL.
};

/**
 * @struct RenderObject
 * @brief Carga útil (payload) enviada a las colas de renderizado que describe exactamente qué y cómo dibujar una instancia.
 */
struct RenderObject {
	Mesh* mesh = nullptr;                               ///< Puntero a la malla con los datos geométricos (vértices/índices).
	MaterialInstance* materialInstance = nullptr;       ///< Puntero al material primario asociado al objeto (obsoleto si usa vector).
	std::vector<MaterialInstance*> materialInstances;   ///< Lista de materiales mapeados a cada sub-malla del Mesh.
	XMMATRIX world = XMMatrixIdentity();                ///< Matriz de transformación local-a-mundo pre-calculada.
	bool castShadow = true;                             ///< Bandera que indica si este objeto debe inyectarse en el pase de sombras.
	bool transparent = false;                           ///< Bandera que redirige el objeto a la cola de geometría Transparente si es verdadero.
	float distanceToCamera = 0.0f;                      ///< Distancia cuadrada calculada a la cámara, usada para z-sorting de objetos transparentes.
};