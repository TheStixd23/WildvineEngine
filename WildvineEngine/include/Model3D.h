#pragma once
#include "Prerequisites.h"
#include "IResource.h"
#include "MeshComponent.h"
#include "fbxsdk.h"


enum ModelType {
    OBJ,
    FBX
};

/**
 * @brief Informacion de un material encontrada dentro de un FBX/OBJ.
 *
 * Las rutas pueden ser absolutas, relativas al modelo o solamente nombres
 * de archivo. BaseApp se encarga de resolverlas contra las carpetas del
 * proyecto antes de crear las texturas de DirectX.
 */
struct ImportedMaterialInfo {
    std::string name = "Default";
    std::string albedoTexture;
    std::string normalTexture;
    std::string metallicTexture;
    std::string roughnessTexture;
    std::string aoTexture;
    std::string emissiveTexture;
    XMFLOAT4 baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    float metallic = 0.0f;
    float roughness = 0.55f;
    float opacity = 1.0f;
};

class Model3D : public IResource {
public:
    Model3D(const std::string& name, ModelType modelType)
        : IResource(name),
          lSdkManager(nullptr),
          lScene(nullptr),
          m_modelType(modelType) {
        SetType(ResourceType::Model3D);
        load(name);
    }

    Model3D(const std::string& name,
        const SkyboxVertex vertices[],
        const unsigned int indices[])
        : IResource(name),
          lSdkManager(nullptr),
          lScene(nullptr),
          m_modelType(FBX) {
        MeshComponent mesh;
        mesh.m_skyVertex.assign(vertices, vertices + 8);
        mesh.m_index.assign(indices, indices + 36);
        mesh.m_numVertex = static_cast<int>(mesh.m_skyVertex.size());
        mesh.m_numIndex = static_cast<int>(mesh.m_index.size());
        SetType(ResourceType::Model3D);
        SetState(ResourceState::Loaded);
        m_meshes.push_back(mesh);
    }

    ~Model3D() override {
        unload();
    }

    bool load(const std::string& path) override;
    bool init() override;
    void unload() override;
    size_t getSizeInBytes() const override;

    const std::vector<MeshComponent>& GetMeshes() const {
        return m_meshes;
    }

    const std::vector<ImportedMaterialInfo>& GetImportedMaterials() const {
        return m_importedMaterials;
    }

    bool InitializeFBXManager();
    std::vector<MeshComponent> LoadFBXModel(const std::string& filePath);
    std::vector<MeshComponent> LoadOBJModel(const std::string& filePath);

    void ProcessFBXNode(FbxNode* node);
    void ProcessFBXMesh(FbxNode* node);
    void ProcessFBXMaterials(FbxSurfaceMaterial* material);

    std::vector<std::string> GetTextureFileNames() const {
        return textureFileNames;
    }

private:
    void releaseFBXResources();
    unsigned int registerFBXMaterial(FbxSurfaceMaterial* material);
    ImportedMaterialInfo extractFBXMaterial(FbxSurfaceMaterial* material) const;
    static std::string getFBXTexturePath(const FbxProperty& property);
    static std::string getFBXTexturePathByPropertyName(
        FbxSurfaceMaterial* material,
        const char* propertyName);

private:
    FbxManager* lSdkManager;
    FbxScene* lScene;
    std::vector<std::string> textureFileNames;
    std::vector<FbxSurfaceMaterial*> m_fbxMaterialPointers;
    std::vector<ImportedMaterialInfo> m_importedMaterials;

    // Conversion global del sistema de ejes original del FBX al sistema
    // DirectX de Wildvine. Se calcula sin modificar la escena importada para
    // evitar aplicar dos veces PreRotation, pivotes o transformaciones locales.
    FbxAMatrix m_sourceToEngineAxis;

    // Factor de unidades desde la unidad original del archivo hacia metros.
    // Se aplica una sola vez junto con la transformacion global de cada nodo.
    double m_sourceToMeters = 1.0;

public:
    ModelType m_modelType;
    std::vector<MeshComponent> m_meshes;
};
