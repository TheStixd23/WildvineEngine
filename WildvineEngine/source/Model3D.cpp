#include "Model3D.h"
#include <cfloat>

bool
Model3D::load(const std::string& path) {
    SetPath(path);
    SetState(ResourceState::Loading);

    init();

    bool success = true; // Cambia esto según el resultado real.

    SetState(success ? ResourceState::Loaded : ResourceState::Failed);
    return success;
}

bool Model3D::init()
{
    // Inicializar recursos GPU, buffers, etc.
    LoadFBXModel(m_filePath);
    return false;
}

void Model3D::unload()
{
    // Liberar buffers, memoria en CPU/GPU, etc.
    SetState(ResourceState::Unloaded);
}

size_t Model3D::getSizeInBytes() const
{
    return 0;
}

bool
Model3D::InitializeFBXManager() {
    // Initialize the FBX SDK manager
    lSdkManager = FbxManager::Create();
    if (!lSdkManager) {
        ERROR("ModelLoader", "FbxManager::Create()", "Unable to create FBX Manager!");
        return false;
    }
    else {
        MESSAGE("ModelLoader", "ModelLoader", "Autodesk FBX SDK version " << lSdkManager->GetVersion())
    }

    // Create an IOSettings object
    FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
    lSdkManager->SetIOSettings(ios);

    // Create an FBX Scene
    lScene = FbxScene::Create(lSdkManager, "MyScene");
    if (!lScene) {
        ERROR("ModelLoader", "FbxScene::Create()", "Unable to create FBX Scene!");
        return false;
    }
    else {
        MESSAGE("ModelLoader", "ModelLoader", "FBX Scene created successfully.")
    }
    return true;
}

std::vector<MeshComponent>
Model3D::LoadFBXModel(const std::string& filePath) {
    m_meshes.clear();

    // 01. Initialize the SDK from FBX Manager
    if (InitializeFBXManager()) {
        // 02. Create an importer using the SDK manager
        FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");
        if (!lImporter) {
            ERROR("ModelLoader", "FbxImporter::Create()", "Unable to create FBX Importer!");
            return std::vector<MeshComponent>();
        }
        else {
            MESSAGE("ModelLoader", "ModelLoader", "FBX Importer created successfully.");
        }

        // 03. Use the first argument as the filename for the importer
        if (!lImporter->Initialize(filePath.c_str(), -1, lSdkManager->GetIOSettings())) {
            ERROR("ModelLoader", "FbxImporter::Initialize()",
                "Unable to initialize FBX Importer! Error: " << lImporter->GetStatus().GetErrorString());
            lImporter->Destroy();
            return std::vector<MeshComponent>();
        }
        else {
            MESSAGE("ModelLoader", "ModelLoader", "FBX Importer initialized successfully.");
        }

        // 04. Import the scene from the file into the scene
        if (!lImporter->Import(lScene)) {
            ERROR("ModelLoader", "FbxImporter::Import()",
                "Unable to import FBX Scene! Error: " << lImporter->GetStatus().GetErrorString());
            lImporter->Destroy();
            return std::vector<MeshComponent>();
        }
        else {
            MESSAGE("ModelLoader", "ModelLoader", "FBX Scene imported successfully.");
            m_name = lImporter->GetFileName();
        }

        FbxAxisSystem::DirectX.ConvertScene(lScene);
        FbxSystemUnit::m.ConvertScene(lScene);
        FbxGeometryConverter gc(lSdkManager);
        gc.Triangulate(lScene, /*replace*/ true);

        // 05. Destroy the importer
        lImporter->Destroy();
        MESSAGE("ModelLoader", "ModelLoader", "FBX Importer destroyed successfully.");

        // 06. Process the model from the scene
        FbxNode* lRootNode = lScene->GetRootNode();

        if (lRootNode) {
            MESSAGE("ModelLoader", "ModelLoader", "Processing model from the scene root node.");
            for (int i = 0; i < lRootNode->GetChildCount(); i++) {
                ProcessFBXNode(lRootNode->GetChild(i));
            }
            return m_meshes;
        }
        else {
            ERROR("ModelLoader", "FbxScene::GetRootNode()",
                "Unable to get root node from FBX Scene!");
            return std::vector<MeshComponent>();
        }
    }
    return m_meshes;
}

void
Model3D::ProcessFBXNode(FbxNode* node) {
    // 01. Process all the node's meshes
    if (node->GetNodeAttribute()) {
        if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh) {
            ProcessFBXMesh(node);
        }
    }

    // 02. Recursively process each child node
    for (int i = 0; i < node->GetChildCount(); i++) {
        ProcessFBXNode(node->GetChild(i));
    }
}


void
Model3D::ProcessFBXMesh(FbxNode* node) {
    FbxMesh* mesh = node ? node->GetMesh() : nullptr;
    if (!mesh) {
        return;
    }

    // ------------------------------------------------------------
    // Transformacion del nodo
    // ------------------------------------------------------------
    FbxAMatrix geometricTransform;
    geometricTransform.SetIdentity();
    geometricTransform.SetT(
        node->GetGeometricTranslation(FbxNode::eSourcePivot));
    geometricTransform.SetR(
        node->GetGeometricRotation(FbxNode::eSourcePivot));
    geometricTransform.SetS(
        node->GetGeometricScaling(FbxNode::eSourcePivot));

    const FbxAMatrix vertexTransform =
        node->EvaluateGlobalTransform() *
        geometricTransform;

    const FbxAMatrix normalTransform =
        vertexTransform.Inverse().Transpose();

    // ------------------------------------------------------------
    // Normales, UV y tangentes
    // ------------------------------------------------------------
    if (mesh->GetElementNormalCount() == 0) {
        mesh->GenerateNormals(true, true);
    }

    FbxStringList uvSets;
    mesh->GetUVSetNames(uvSets);
    const char* uvSetName =
        uvSets.GetCount() > 0
        ? uvSets[0]
        : nullptr;

    if (mesh->GetElementTangentCount() == 0 &&
        uvSetName) {
        mesh->GenerateTangentsData(uvSetName);
    }

    const FbxGeometryElementUV* uvElement =
        mesh->GetElementUVCount() > 0
        ? mesh->GetElementUV(0)
        : nullptr;

    const FbxGeometryElementTangent* tangentElement =
        mesh->GetElementTangentCount() > 0
        ? mesh->GetElementTangent(0)
        : nullptr;

    const FbxGeometryElementBinormal* binormalElement =
        mesh->GetElementBinormalCount() > 0
        ? mesh->GetElementBinormal(0)
        : nullptr;

    const FbxGeometryElementMaterial* materialElement =
        mesh->GetElementMaterial();

    auto toLower =
        [](std::string text) {
            for (char& character : text) {
                if (character >= 'A' &&
                    character <= 'Z') {
                    character =
                        static_cast<char>(
                            character + ('a' - 'A'));
                }
            }
            return text;
        };

    const std::string nodeName =
        node->GetName()
        ? node->GetName()
        : "Mesh";

    const std::string lowerNodeName =
        toLower(nodeName);

    const bool isTireAssembly =
        lowerNodeName.find("_tireb_") !=
            std::string::npos ||
        lowerNodeName.find("sidewall") !=
            std::string::npos;

    // ------------------------------------------------------------
    // El FBX del Alfa Romeo contiene dos ruedas dentro de cada
    // geometria de eje:
    //
    // - Una rueda se encuentra cerca del origen local.
    // - La rueda contraria aparece desplazada 6-8 metros.
    //
    // El visor original recoloca esa segunda mitad. El cargador anterior
    // no lo hacia y por eso las ruedas aparecian flotando.
    //
    // Detectamos el gran salto en X y calculamos una correccion para que
    // la rueda lejana quede simetrica respecto al centro del automovil.
    // ------------------------------------------------------------
    bool hasSeparatedTireCopy = false;
    double tireSplitX = 0.0;
    FbxVector4 farWheelWorldCorrection(
        0.0, 0.0, 0.0, 0.0);

    if (isTireAssembly &&
        mesh->GetControlPointsCount() > 1) {

        std::vector<double> xValues;
        xValues.reserve(
            mesh->GetControlPointsCount());

        for (int controlPointIndex = 0;
            controlPointIndex <
                mesh->GetControlPointsCount();
            ++controlPointIndex) {

            xValues.push_back(
                mesh->GetControlPointAt(
                    controlPointIndex)[0]);
        }

        std::sort(
            xValues.begin(),
            xValues.end());

        double largestGap = 0.0;
        size_t largestGapIndex = 0;

        for (size_t valueIndex = 0;
            valueIndex + 1 < xValues.size();
            ++valueIndex) {

            const double gap =
                xValues[valueIndex + 1] -
                xValues[valueIndex];

            if (gap > largestGap) {
                largestGap = gap;
                largestGapIndex = valueIndex;
            }
        }

        // El salto real del modelo supera ampliamente un metro.
        if (largestGap > 1.0) {
            hasSeparatedTireCopy = true;

            tireSplitX =
                (xValues[largestGapIndex] +
                    xValues[largestGapIndex + 1]) *
                0.5;

            FbxVector4 nearMinimum(
                DBL_MAX, DBL_MAX, DBL_MAX, 1.0);
            FbxVector4 nearMaximum(
                -DBL_MAX, -DBL_MAX, -DBL_MAX, 1.0);
            FbxVector4 farMinimum(
                DBL_MAX, DBL_MAX, DBL_MAX, 1.0);
            FbxVector4 farMaximum(
                -DBL_MAX, -DBL_MAX, -DBL_MAX, 1.0);

            for (int controlPointIndex = 0;
                controlPointIndex <
                    mesh->GetControlPointsCount();
                ++controlPointIndex) {

                const FbxVector4 point =
                    mesh->GetControlPointAt(
                        controlPointIndex);

                FbxVector4& minimum =
                    point[0] <= tireSplitX
                    ? nearMinimum
                    : farMinimum;

                FbxVector4& maximum =
                    point[0] <= tireSplitX
                    ? nearMaximum
                    : farMaximum;

                for (int axis = 0;
                    axis < 3;
                    ++axis) {

                    minimum[axis] =
                        FbxMin(
                            minimum[axis],
                            point[axis]);

                    maximum[axis] =
                        FbxMax(
                            maximum[axis],
                            point[axis]);
                }
            }

            const FbxVector4 nearCenterLocal(
                (nearMinimum[0] +
                    nearMaximum[0]) * 0.5,
                (nearMinimum[1] +
                    nearMaximum[1]) * 0.5,
                (nearMinimum[2] +
                    nearMaximum[2]) * 0.5,
                1.0);

            const FbxVector4 farCenterLocal(
                (farMinimum[0] +
                    farMaximum[0]) * 0.5,
                (farMinimum[1] +
                    farMaximum[1]) * 0.5,
                (farMinimum[2] +
                    farMaximum[2]) * 0.5,
                1.0);

            const FbxVector4 nearCenterWorld =
                vertexTransform.MultT(
                    nearCenterLocal);

            const FbxVector4 farCenterWorld =
                vertexTransform.MultT(
                    farCenterLocal);

            // Posicion deseada: espejo de la rueda cercana
            // alrededor del plano central X = 0.
            const FbxVector4 desiredFarCenterWorld(
                -nearCenterWorld[0],
                nearCenterWorld[1],
                nearCenterWorld[2],
                1.0);

            farWheelWorldCorrection =
                desiredFarCenterWorld -
                farCenterWorld;

            farWheelWorldCorrection[3] = 0.0;

            const std::wstring nodeNameWide(
                nodeName.begin(),
                nodeName.end());

            MESSAGE(
                "ModelLoader",
                "ProcessFBXMesh",
                L"Corrigiendo pareja de ruedas: "
                << nodeNameWide);
        }
    }

    // ------------------------------------------------------------
    // Separa la geometria por material del FBX.
    // Esto es indispensable para que llanta, rin, cromo y piezas negras
    // no reciban el mismo material.
    // ------------------------------------------------------------
    int materialCount =
        node->GetMaterialCount();

    if (materialCount < 1) {
        materialCount = 1;
    }

    struct MeshBuilder {
        std::string name;
        std::vector<SimpleVertex> vertices;
        std::vector<unsigned int> indices;
    };

    std::vector<MeshBuilder> builders(
        static_cast<size_t>(
            materialCount));

    for (int materialIndex = 0;
        materialIndex < materialCount;
        ++materialIndex) {

        std::string materialName =
            "Default";

        if (materialIndex <
            node->GetMaterialCount()) {

            FbxSurfaceMaterial* material =
                node->GetMaterial(
                    materialIndex);

            if (material &&
                material->GetName()) {
                materialName =
                    material->GetName();
            }
        }

        builders[materialIndex].name =
            nodeName +
            "__mat_" +
            materialName;
    }

    auto getPolygonMaterialIndex =
        [materialElement,
            materialCount](
                int polygonIndex) {

            if (!materialElement ||
                materialCount <= 1) {
                return 0;
            }

            int materialIndex = 0;

            switch (
                materialElement->
                    GetMappingMode()) {

            case FbxGeometryElement::eByPolygon:
                if (materialElement->
                    GetReferenceMode() ==
                    FbxGeometryElement::
                        eIndexToDirect) {

                    materialIndex =
                        materialElement->
                            GetIndexArray().
                            GetAt(
                                polygonIndex);
                }
                else {
                    materialIndex =
                        polygonIndex;
                }
                break;

            case FbxGeometryElement::eAllSame:
            default:
                if (materialElement->
                    GetIndexArray().
                    GetCount() > 0) {

                    materialIndex =
                        materialElement->
                            GetIndexArray().
                            GetAt(0);
                }
                break;
            }

            if (materialIndex < 0 ||
                materialIndex >=
                    materialCount) {
                materialIndex = 0;
            }

            return materialIndex;
        };

    auto readVector4 =
        [](const auto* element,
            int controlPointIndex,
            int polygonVertexIndex) {

            if (!element) {
                return FbxVector4(
                    0.0, 0.0, 0.0, 0.0);
            }

            int elementIndex = 0;

            if (element->
                GetMappingMode() ==
                FbxGeometryElement::
                    eByControlPoint) {

                elementIndex =
                    controlPointIndex;
            }
            else {
                elementIndex =
                    polygonVertexIndex;
            }

            if (element->
                GetReferenceMode() ==
                FbxGeometryElement::
                    eIndexToDirect) {

                elementIndex =
                    element->
                        GetIndexArray().
                        GetAt(
                            elementIndex);
            }

            return element->
                GetDirectArray().
                GetAt(
                    elementIndex);
        };

    // ------------------------------------------------------------
    // Construccion de triangulos por material
    // ------------------------------------------------------------
    for (int polygonIndex = 0;
        polygonIndex <
            mesh->GetPolygonCount();
        ++polygonIndex) {

        const int polygonSize =
            mesh->GetPolygonSize(
                polygonIndex);

        if (polygonSize < 3) {
            continue;
        }

        const int materialIndex =
            getPolygonMaterialIndex(
                polygonIndex);

        MeshBuilder& builder =
            builders[
                static_cast<size_t>(
                    materialIndex)];

        std::vector<unsigned int>
            polygonCorners;

        polygonCorners.reserve(
            static_cast<size_t>(
                polygonSize));

        for (int polygonCorner = 0;
            polygonCorner <
                polygonSize;
            ++polygonCorner) {

            const int controlPointIndex =
                mesh->GetPolygonVertex(
                    polygonIndex,
                    polygonCorner);

            const int polygonVertexIndex =
                mesh->
                    GetPolygonVertexIndex(
                        polygonIndex) +
                polygonCorner;

            const FbxVector4 localPosition =
                mesh->GetControlPointAt(
                    controlPointIndex);

            const bool belongsToFarWheel =
                hasSeparatedTireCopy &&
                localPosition[0] >
                    tireSplitX;

            FbxVector4 worldPosition =
                vertexTransform.MultT(
                    localPosition);

            if (belongsToFarWheel) {
                worldPosition +=
                    farWheelWorldCorrection;
            }

            SimpleVertex vertex{};

            vertex.Position =
                EU::Vector3(
                    static_cast<float>(
                        worldPosition[0]),
                    static_cast<float>(
                        worldPosition[1]),
                    static_cast<float>(
                        worldPosition[2]));

            FbxVector4 normal(
                0.0, 1.0, 0.0, 0.0);

            mesh->
                GetPolygonVertexNormal(
                    polygonIndex,
                    polygonCorner,
                    normal);

            normal[3] = 0.0;
            normal =
                normalTransform.MultT(
                    normal);
            normal[3] = 0.0;
            normal.Normalize();

            vertex.Normal =
                EU::Vector3(
                    static_cast<float>(
                        normal[0]),
                    static_cast<float>(
                        normal[1]),
                    static_cast<float>(
                        normal[2]));

            FbxVector2 uv(0.0, 0.0);

            if (uvSetName) {
                bool unmapped = false;

                mesh->
                    GetPolygonVertexUV(
                        polygonIndex,
                        polygonCorner,
                        uvSetName,
                        uv,
                        unmapped);

                if (unmapped) {
                    uv = FbxVector2(
                        0.0, 0.0);
                }
            }

            vertex.TextureCoordinate =
                EU::Vector2(
                    static_cast<float>(
                        uv[0]),
                    1.0f -
                    static_cast<float>(
                        uv[1]));

            if (tangentElement) {
                FbxVector4 tangent =
                    readVector4(
                        tangentElement,
                        controlPointIndex,
                        polygonVertexIndex);

                tangent[3] = 0.0;
                tangent =
                    normalTransform.MultT(
                        tangent);
                tangent[3] = 0.0;
                tangent.Normalize();

                vertex.Tangent =
                    EU::Vector3(
                        static_cast<float>(
                            tangent[0]),
                        static_cast<float>(
                            tangent[1]),
                        static_cast<float>(
                            tangent[2]));
            }
            else {
                vertex.Tangent =
                    EU::Vector3(
                        0.0f, 0.0f, 0.0f);
            }

            if (binormalElement) {
                FbxVector4 binormal =
                    readVector4(
                        binormalElement,
                        controlPointIndex,
                        polygonVertexIndex);

                binormal[3] = 0.0;
                binormal =
                    normalTransform.MultT(
                        binormal);
                binormal[3] = 0.0;
                binormal.Normalize();

                vertex.Bitangent =
                    EU::Vector3(
                        static_cast<float>(
                            binormal[0]),
                        static_cast<float>(
                            binormal[1]),
                        static_cast<float>(
                            binormal[2]));
            }
            else {
                vertex.Bitangent =
                    EU::Vector3(
                        0.0f, 0.0f, 0.0f);
            }

            polygonCorners.push_back(
                static_cast<unsigned int>(
                    builder.vertices.size()));

            builder.vertices.push_back(
                vertex);
        }

        // Winding horario, igual que el cargador anterior.
        for (int triangleIndex = 1;
            triangleIndex + 1 <
                polygonSize;
            ++triangleIndex) {

            builder.indices.push_back(
                polygonCorners[0]);

            builder.indices.push_back(
                polygonCorners[
                    triangleIndex + 1]);

            builder.indices.push_back(
                polygonCorners[
                    triangleIndex]);
        }
    }

    // ------------------------------------------------------------
    // Tangentes fallback y empaquetado
    // ------------------------------------------------------------
    auto addVector =
        [](EU::Vector3 left,
            const EU::Vector3& right) {

            left.x += right.x;
            left.y += right.y;
            left.z += right.z;
            return left;
        };

    auto subtractVector =
        [](const EU::Vector3& left,
            const EU::Vector3& right) {

            return EU::Vector3(
                left.x - right.x,
                left.y - right.y,
                left.z - right.z);
        };

    auto multiplyVector =
        [](const EU::Vector3& vector,
            float value) {

            return EU::Vector3(
                vector.x * value,
                vector.y * value,
                vector.z * value);
        };

    auto dotVector =
        [](const EU::Vector3& left,
            const EU::Vector3& right) {

            return
                left.x * right.x +
                left.y * right.y +
                left.z * right.z;
        };

    auto crossVector =
        [](const EU::Vector3& left,
            const EU::Vector3& right) {

            return EU::Vector3(
                left.y * right.z -
                    left.z * right.y,
                left.z * right.x -
                    left.x * right.z,
                left.x * right.y -
                    left.y * right.x);
        };

    auto normalizeVector =
        [](EU::Vector3& vector) {

            const float length =
                sqrtf(
                    EU::EMax(
                        1e-20f,
                        vector.x * vector.x +
                        vector.y * vector.y +
                        vector.z * vector.z));

            vector.x /= length;
            vector.y /= length;
            vector.z /= length;
        };

    for (MeshBuilder& builder :
        builders) {

        if (builder.vertices.empty() ||
            builder.indices.empty()) {
            continue;
        }

        if (!tangentElement ||
            !binormalElement) {

            for (size_t index = 0;
                index + 2 <
                    builder.indices.size();
                index += 3) {

                SimpleVertex& vertex0 =
                    builder.vertices[
                        builder.indices[
                            index + 0]];

                SimpleVertex& vertex1 =
                    builder.vertices[
                        builder.indices[
                            index + 1]];

                SimpleVertex& vertex2 =
                    builder.vertices[
                        builder.indices[
                            index + 2]];

                const EU::Vector3 edge1 =
                    subtractVector(
                        vertex1.Position,
                        vertex0.Position);

                const EU::Vector3 edge2 =
                    subtractVector(
                        vertex2.Position,
                        vertex0.Position);

                const float deltaU1 =
                    vertex1.
                        TextureCoordinate.x -
                    vertex0.
                        TextureCoordinate.x;

                const float deltaV1 =
                    vertex1.
                        TextureCoordinate.y -
                    vertex0.
                        TextureCoordinate.y;

                const float deltaU2 =
                    vertex2.
                        TextureCoordinate.x -
                    vertex0.
                        TextureCoordinate.x;

                const float deltaV2 =
                    vertex2.
                        TextureCoordinate.y -
                    vertex0.
                        TextureCoordinate.y;

                const float denominator =
                    deltaU1 * deltaV2 -
                    deltaU2 * deltaV1;

                const float inverse =
                    fabsf(denominator) <
                        1e-8f
                    ? 0.0f
                    : 1.0f /
                        denominator;

                const EU::Vector3 tangent =
                    multiplyVector(
                        EU::Vector3(
                            edge1.x * deltaV2 -
                                edge2.x * deltaV1,
                            edge1.y * deltaV2 -
                                edge2.y * deltaV1,
                            edge1.z * deltaV2 -
                                edge2.z * deltaV1),
                        inverse);

                const EU::Vector3 bitangent =
                    multiplyVector(
                        EU::Vector3(
                            edge2.x * deltaU1 -
                                edge1.x * deltaU2,
                            edge2.y * deltaU1 -
                                edge1.y * deltaU2,
                            edge2.z * deltaU1 -
                                edge1.z * deltaU2),
                        inverse);

                vertex0.Tangent =
                    addVector(
                        vertex0.Tangent,
                        tangent);
                vertex1.Tangent =
                    addVector(
                        vertex1.Tangent,
                        tangent);
                vertex2.Tangent =
                    addVector(
                        vertex2.Tangent,
                        tangent);

                vertex0.Bitangent =
                    addVector(
                        vertex0.Bitangent,
                        bitangent);
                vertex1.Bitangent =
                    addVector(
                        vertex1.Bitangent,
                        bitangent);
                vertex2.Bitangent =
                    addVector(
                        vertex2.Bitangent,
                        bitangent);
            }
        }

        for (SimpleVertex& vertex :
            builder.vertices) {

            normalizeVector(
                vertex.Normal);

            const float tangentProjection =
                dotVector(
                    vertex.Tangent,
                    vertex.Normal);

            vertex.Tangent =
                subtractVector(
                    vertex.Tangent,
                    EU::Vector3(
                        vertex.Normal.x *
                            tangentProjection,
                        vertex.Normal.y *
                            tangentProjection,
                        vertex.Normal.z *
                            tangentProjection));

            normalizeVector(
                vertex.Tangent);

            EU::Vector3 calculatedBitangent =
                crossVector(
                    vertex.Normal,
                    vertex.Tangent);

            const float handedness =
                dotVector(
                    calculatedBitangent,
                    vertex.Bitangent) <
                    0.0f
                ? -1.0f
                : 1.0f;

            vertex.Bitangent =
                multiplyVector(
                    calculatedBitangent,
                    handedness);

            normalizeVector(
                vertex.Bitangent);
        }

        MeshComponent meshComponent;
        meshComponent.m_name =
            builder.name;
        meshComponent.m_vertex =
            std::move(
                builder.vertices);
        meshComponent.m_index =
            std::move(
                builder.indices);
        meshComponent.m_numVertex =
            static_cast<int>(
                meshComponent.
                    m_vertex.size());
        meshComponent.m_numIndex =
            static_cast<int>(
                meshComponent.
                    m_index.size());

        m_meshes.push_back(
            std::move(
                meshComponent));
    }
}


void Model3D::ProcessFBXMaterials(FbxSurfaceMaterial* material)
{
    if (material) {
        FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
        if (prop.IsValid()) {
            int textureCount = prop.GetSrcObjectCount<FbxTexture>();
            for (int i = 0; i < textureCount; ++i) {
                FbxTexture* texture = FbxCast<FbxTexture>(prop.GetSrcObject<FbxTexture>(i));
                if (texture) {
                    textureFileNames.push_back(texture->GetName());
                }
            }
        }
    }
}
