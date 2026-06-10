/**
 * @file Model3D.h
 * @brief Declara la API de Model3D dentro del subsistema Core de WildvineEngine.
 * @ingroup core
 */
#pragma once
#include "Prerequisites.h"
#include "IResource.h"
#include "MeshComponent.h"
#include "fbxsdk.h"

 /**
  * @enum ModelType
  * @brief Identifica el formato del archivo de origen para el modelo 3D.
  */
enum
	ModelType {
	OBJ, ///< Formato Wavefront OBJ (texto/geometría básica).
	FBX  ///< Formato Autodesk FBX (soporta jerarquías y materiales complejos).
};

/**
 * @class Model3D
 * @brief Recurso que gestiona la carga, parseo y almacenamiento de geometría 3D.
 *
 * `Model3D` hereda de `IResource` para integrarse en el sistema general de recursos.
 * Soporta la importación de archivos FBX y OBJ extrayendo vértices, índices y
 * referencias a texturas. También implementa un sistema de caché binario local
 * para acelerar significativamente los tiempos de carga en ejecuciones posteriores.
 */
class
	Model3D : public IResource {
public:
	/**
	 * @brief Constructor principal para inicializar un modelo desde un archivo de disco.
	 * @param name Nombre identificador único del recurso.
	 * @param modelType Formato esperado del archivo origen (OBJ o FBX).
	 */
	Model3D(const std::string& name, ModelType modelType)
		: IResource(name), m_modelType(modelType), lSdkManager(nullptr), lScene(nullptr) {
		SetType(ResourceType::Model3D);
	}

	/**
	 * @brief Constructor especializado para generar la geometría del Skybox a partir de arreglos en código.
	 * @param name Nombre identificador del recurso.
	 * @param vertices Arreglo de vértices predefinidos del cubo del Skybox.
	 * @param indices Arreglo de índices para dibujar el cubo.
	 */
	Model3D(const std::string& name,
		const SkyboxVertex vertices[],
		const unsigned int indices[]) : IResource(name) {
		MeshComponent mesh;
		mesh.m_skyVertex.assign(vertices, vertices + 8);
		mesh.m_index.assign(indices, indices + 36);
		mesh.m_numIndex = mesh.m_index.size();
		SetType(ResourceType::Model3D);
		m_meshes.push_back(mesh);
	}

	/**
	 * @brief Destructor que asegura la liberación segura del SDK de FBX y la memoria de las mallas.
	 */
	~Model3D() override;

	/**
	 * @brief Carga los datos del modelo desde la ruta, utilizando el caché binario si está disponible y actualizado.
	 * @param path Ruta del archivo a importar.
	 * @return true Si el modelo se cargó exitosamente en RAM.
	 */
	bool
		load(const std::string& path) override;

	/**
	 * @brief Prepara o inicializa los datos del modelo en la memoria (implementación de la interfaz IResource).
	 * @return true Si la inicialización fue exitosa.
	 */
	bool
		init() override;

	/**
	 * @brief Descarga los datos del modelo de la memoria y limpia el arreglo de mallas.
	 */
	void
		unload() override;

	/**
	 * @brief Calcula el tamaño estimado en bytes que ocupa este recurso completo en RAM.
	 * @return size_t Tamaño en bytes.
	 */
	size_t
		getSizeInBytes() const override;

	/**
	 * @brief Obtiene la colección de submallas parseadas listas para ser renderizadas.
	 * @return const std::vector<MeshComponent>& Referencia a la lista de componentes de malla.
	 */
	const std::vector<MeshComponent>&
		GetMeshes() const { return m_meshes; }

	/* FBX MODEL LOADER*/

	/**
	 * @brief Inicializa las estructuras base y el administrador del SDK de Autodesk FBX.
	 * @return true Si el SDK se inicializa sin errores.
	 */
	bool
		InitializeFBXManager();

	/**
	 * @brief Importa un archivo FBX, construyendo la escena en memoria y procesando sus nodos.
	 * @param filePath Ruta del archivo FBX original.
	 * @return std::vector<MeshComponent> Arreglo con la geometría extraída.
	 */
	std::vector<MeshComponent>
		LoadFBXModel(const std::string& filePath);

	/**
	 * @brief Importa un archivo OBJ básico (geometría y UVs).
	 * @param filePath Ruta del archivo OBJ.
	 * @return std::vector<MeshComponent> Arreglo con la geometría extraída.
	 */
	std::vector<MeshComponent>
		LoadOBJModel(const std::string& filePath);

	/**
	 * @brief Procesa de manera recursiva un nodo dentro del árbol de la escena FBX.
	 * @param node Puntero al nodo FBX actual.
	 */
	void
		ProcessFBXNode(FbxNode* node);

	/**
	 * @brief Extrae los datos geométricos (vértices, normales, tangentes, UVs) de un nodo Mesh en FBX.
	 * @param node Puntero al nodo FBX evaluado.
	 */
	void
		ProcessFBXMesh(FbxNode* node);

	/**
	 * @brief Extrae las referencias o rutas de texturas vinculadas a un material FBX.
	 * @param material Puntero a las propiedades del material de superficie FBX.
	 */
	void
		ProcessFBXMaterials(FbxSurfaceMaterial* material);

	/**
	 * @brief Obtiene las rutas o nombres de archivo de las texturas descubiertas durante el parseo de materiales.
	 * @return std::vector<std::string> Lista de nombres de archivos.
	 */
	std::vector<std::string>
		GetTextureFileNames() const { return textureFileNames; }

private:
	/** @brief Genera una ruta en disco para almacenar el archivo binario (.bin) asociado a este modelo. */
	std::string GetBinaryCachePath() const;
	/** @brief Verifica si el archivo de caché binario existe y si fue modificado después que el archivo origen. */
	bool IsBinaryCacheUpToDate(const std::string& sourcePath, const std::string& cachePath) const;
	/** @brief Restaura rápidamente las estructuras de mallas directamente desde disco. */
	bool LoadBinaryCache(const std::string& cachePath);
	/** @brief Escribe las mallas recién parseadas en un bloque continuo de datos en disco para optimizar futuras cargas. */
	bool SaveBinaryCache(const std::string& cachePath) const;

private:
	FbxManager* lSdkManager;                         ///< Instancia principal del sistema interno del SDK de FBX.
	FbxScene* lScene;                                ///< Grafo lógico contenedor de toda la información importada del archivo FBX.
	std::vector<std::string> textureFileNames;       ///< Lista temporal de archivos de textura detectados en el parseo.
public:
	ModelType m_modelType;                           ///< Identificador del tipo de archivo (OBJ o FBX).
	std::vector<MeshComponent> m_meshes;             ///< Colección de la geometría final construida tras el parseo.
};