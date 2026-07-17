#include "Model3D.h"
#include <cfloat>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <limits>

namespace {
    struct ObjIndex {
        int position = 0;
        int texcoord = 0;
        int normal = 0;
    };

    std::string TrimText(const std::string& value) {
        const size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return std::string();
        }

        const size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    std::string DirectoryOfFile(const std::string& path) {
        const size_t separator = path.find_last_of("/\\");
        return separator == std::string::npos
            ? std::string()
            : path.substr(0, separator);
    }

    std::string JoinFilePath(
        const std::string& directory,
        const std::string& fileName) {

        if (fileName.empty()) {
            return std::string();
        }

        if (fileName.size() > 1 && fileName[1] == ':') {
            return fileName;
        }

        if (fileName[0] == '/' || fileName[0] == '\\') {
            return fileName;
        }

        if (directory.empty()) {
            return fileName;
        }

        const char last = directory[directory.size() - 1];
        return directory +
            ((last == '/' || last == '\\') ? "" : "/") +
            fileName;
    }

    std::string ParseMtlTextureReference(const std::string& value) {
        std::string result = TrimText(value);
        if (result.empty()) {
            return result;
        }

        if (result.front() == '"' && result.back() == '"' &&
            result.size() >= 2) {
            return result.substr(1, result.size() - 2);
        }

        // Las opciones MTL (-s, -o, -bm, etc.) preceden a la ruta. Cuando
        // existen, la ruta suele ser el ultimo argumento de la linea.
        if (result[0] == '-') {
            std::istringstream tokens(result);
            std::string token;
            std::string lastToken;
            while (tokens >> token) {
                lastToken = token;
            }
            result = lastToken;
        }

        return result;
    }

    bool LoadMtlLibrary(
        const std::string& libraryPath,
        std::vector<ImportedMaterialInfo>& outMaterials,
        std::unordered_map<std::string, unsigned int>& outSlots) {

        std::ifstream file(libraryPath.c_str());
        if (!file.is_open()) {
            return false;
        }

        const std::string materialDirectory =
            DirectoryOfFile(libraryPath);

        ImportedMaterialInfo current;
        bool hasCurrent = false;
        bool parsedAnyMaterial = false;

        auto commitCurrent = [&]() {
            if (!hasCurrent || current.name.empty()) {
                return;
            }

            current.metallic =
                (std::max)(0.0f, (std::min)(1.0f, current.metallic));
            current.roughness =
                (std::max)(0.02f, (std::min)(1.0f, current.roughness));
            current.opacity =
                (std::max)(0.0f, (std::min)(1.0f, current.opacity));
            current.baseColor.x =
                (std::max)(0.0f, (std::min)(1.0f, current.baseColor.x));
            current.baseColor.y =
                (std::max)(0.0f, (std::min)(1.0f, current.baseColor.y));
            current.baseColor.z =
                (std::max)(0.0f, (std::min)(1.0f, current.baseColor.z));
            current.baseColor.w = current.opacity;

            parsedAnyMaterial = true;

            const auto existing = outSlots.find(current.name);
            if (existing != outSlots.end()) {
                outMaterials[existing->second] = current;
                return;
            }

            const unsigned int slot =
                static_cast<unsigned int>(outMaterials.size());
            outSlots[current.name] = slot;
            outMaterials.push_back(current);
        };

        std::string line;
        while (std::getline(file, line)) {
            line = TrimText(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream stream(line);
            std::string command;
            stream >> command;

            std::string rest;
            std::getline(stream, rest);
            rest = TrimText(rest);

            if (command == "newmtl") {
                commitCurrent();
                current = ImportedMaterialInfo();
                current.name = rest.empty() ? "Default" : rest;
                hasCurrent = true;
            }
            else if (!hasCurrent) {
                continue;
            }
            else if (command == "Kd") {
                std::istringstream values(rest);
                values >> current.baseColor.x
                    >> current.baseColor.y
                    >> current.baseColor.z;
            }
            else if (command == "d") {
                std::istringstream values(rest);
                values >> current.opacity;
                current.baseColor.w = current.opacity;
            }
            else if (command == "Tr") {
                float transparency = 0.0f;
                std::istringstream values(rest);
                values >> transparency;
                current.opacity = 1.0f - transparency;
                current.baseColor.w = current.opacity;
            }
            else if (command == "Ns") {
                float shininess = 0.0f;
                std::istringstream values(rest);
                values >> shininess;
                shininess = (std::max)(0.0f, shininess);
                current.roughness = sqrtf(2.0f / (shininess + 2.0f));
            }
            else if (command == "Pm") {
                std::istringstream values(rest);
                values >> current.metallic;
            }
            else if (command == "Pr") {
                std::istringstream values(rest);
                values >> current.roughness;
            }
            else {
                const std::string textureReference =
                    ParseMtlTextureReference(rest);
                const std::string texturePath =
                    JoinFilePath(materialDirectory, textureReference);

                if (command == "map_Kd") {
                    current.albedoTexture = texturePath;
                }
                else if (command == "map_Bump" ||
                    command == "map_bump" ||
                    command == "bump" ||
                    command == "norm" ||
                    command == "map_Kn") {
                    current.normalTexture = texturePath;
                }
                else if (command == "map_Pm" ||
                    command == "map_metallic") {
                    current.metallicTexture = texturePath;
                }
                else if (command == "map_Pr" ||
                    command == "map_roughness") {
                    current.roughnessTexture = texturePath;
                }
                else if (command == "map_AO" ||
                    command == "map_ao" ||
                    command == "map_occlusion") {
                    current.aoTexture = texturePath;
                }
                else if (command == "map_Ke" ||
                    command == "map_emissive") {
                    current.emissiveTexture = texturePath;
                }
            }
        }

        commitCurrent();
        return parsedAnyMaterial;
    }

    int ResolveObjIndex(int value, size_t count) {
        if (value > 0) return value - 1;
        if (value < 0) return static_cast<int>(count) + value;
        return -1;
    }

    bool ParseObjIndex(const std::string& token, ObjIndex& out) {
        out = ObjIndex{};
        try {
            const size_t firstSlash = token.find('/');
            if (firstSlash == std::string::npos) {
                out.position = std::stoi(token);
                return true;
            }

            const std::string positionText = token.substr(0, firstSlash);
            if (positionText.empty()) return false;
            out.position = std::stoi(positionText);

            const size_t secondSlash = token.find('/', firstSlash + 1);
            if (secondSlash == std::string::npos) {
                const std::string texcoordText = token.substr(firstSlash + 1);
                if (!texcoordText.empty()) out.texcoord = std::stoi(texcoordText);
                return true;
            }

            const std::string texcoordText =
                token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
            const std::string normalText = token.substr(secondSlash + 1);

            if (!texcoordText.empty()) out.texcoord = std::stoi(texcoordText);
            if (!normalText.empty()) out.normal = std::stoi(normalText);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    void FinalizeObjMesh(MeshComponent& mesh) {
        if (mesh.m_vertex.empty() || mesh.m_index.empty()) return;

        std::vector<EU::Vector3> generatedNormals(mesh.m_vertex.size());
        std::vector<EU::Vector3> generatedTangents(mesh.m_vertex.size());
        std::vector<EU::Vector3> generatedBitangents(mesh.m_vertex.size());

        for (size_t index = 0; index + 2 < mesh.m_index.size(); index += 3) {
            const unsigned int i0 = mesh.m_index[index + 0];
            const unsigned int i1 = mesh.m_index[index + 1];
            const unsigned int i2 = mesh.m_index[index + 2];
            if (i0 >= mesh.m_vertex.size() ||
                i1 >= mesh.m_vertex.size() ||
                i2 >= mesh.m_vertex.size()) {
                continue;
            }

            SimpleVertex& v0 = mesh.m_vertex[i0];
            SimpleVertex& v1 = mesh.m_vertex[i1];
            SimpleVertex& v2 = mesh.m_vertex[i2];

            const EU::Vector3 edge1 = v1.Position - v0.Position;
            const EU::Vector3 edge2 = v2.Position - v0.Position;
            const EU::Vector3 faceNormal = EU::Vector3::cross(edge1, edge2).normalize();

            generatedNormals[i0] += faceNormal;
            generatedNormals[i1] += faceNormal;
            generatedNormals[i2] += faceNormal;

            const float deltaU1 = v1.TextureCoordinate.x - v0.TextureCoordinate.x;
            const float deltaV1 = v1.TextureCoordinate.y - v0.TextureCoordinate.y;
            const float deltaU2 = v2.TextureCoordinate.x - v0.TextureCoordinate.x;
            const float deltaV2 = v2.TextureCoordinate.y - v0.TextureCoordinate.y;
            const float denominator = deltaU1 * deltaV2 - deltaU2 * deltaV1;

            if (fabsf(denominator) > 0.000001f) {
                const float inverse = 1.0f / denominator;
                const EU::Vector3 tangent(
                    (edge1.x * deltaV2 - edge2.x * deltaV1) * inverse,
                    (edge1.y * deltaV2 - edge2.y * deltaV1) * inverse,
                    (edge1.z * deltaV2 - edge2.z * deltaV1) * inverse);
                const EU::Vector3 bitangent(
                    (edge2.x * deltaU1 - edge1.x * deltaU2) * inverse,
                    (edge2.y * deltaU1 - edge1.y * deltaU2) * inverse,
                    (edge2.z * deltaU1 - edge1.z * deltaU2) * inverse);

                generatedTangents[i0] += tangent;
                generatedTangents[i1] += tangent;
                generatedTangents[i2] += tangent;
                generatedBitangents[i0] += bitangent;
                generatedBitangents[i1] += bitangent;
                generatedBitangents[i2] += bitangent;
            }
        }

        for (size_t vertexIndex = 0; vertexIndex < mesh.m_vertex.size(); ++vertexIndex) {
            SimpleVertex& vertex = mesh.m_vertex[vertexIndex];
            if (vertex.Normal.isNearlyZero()) {
                vertex.Normal = generatedNormals[vertexIndex].normalize();
            }
            else {
                vertex.Normal = vertex.Normal.normalize();
            }

            vertex.Tangent = generatedTangents[vertexIndex].normalize();
            if (vertex.Tangent.isNearlyZero()) {
                const EU::Vector3 reference =
                    fabsf(vertex.Normal.y) < 0.99f
                    ? EU::Vector3(0.0f, 1.0f, 0.0f)
                    : EU::Vector3(1.0f, 0.0f, 0.0f);
                vertex.Tangent = EU::Vector3::cross(reference, vertex.Normal).normalize();
            }

            vertex.Bitangent = generatedBitangents[vertexIndex].normalize();
            if (vertex.Bitangent.isNearlyZero()) {
                vertex.Bitangent = EU::Vector3::cross(vertex.Normal, vertex.Tangent).normalize();
            }
        }

        mesh.m_numVertex = static_cast<int>(mesh.m_vertex.size());
        mesh.m_numIndex = static_cast<int>(mesh.m_index.size());
    }


    FbxAMatrix GetFBXGeometryTransform(FbxNode* node) {
        FbxAMatrix geometry;
        geometry.SetIdentity();
        if (!node) {
            return geometry;
        }

        geometry.SetT(
            node->GetGeometricTranslation(
                FbxNode::eSourcePivot));
        geometry.SetR(
            node->GetGeometricRotation(
                FbxNode::eSourcePivot));
        geometry.SetS(
            node->GetGeometricScaling(
                FbxNode::eSourcePivot));
        return geometry;
    }

    void MatrixSetZero(FbxAMatrix& matrix) {
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                matrix[row][column] = 0.0;
            }
        }
    }

    void MatrixScale(FbxAMatrix& matrix, double value) {
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                matrix[row][column] *= value;
            }
        }
    }

    void MatrixAdd(FbxAMatrix& destination, const FbxAMatrix& source) {
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                destination[row][column] += source[row][column];
            }
        }
    }

    void MatrixAddToDiagonal(FbxAMatrix& matrix, double value) {
        matrix[0][0] += value;
        matrix[1][1] += value;
        matrix[2][2] += value;
        matrix[3][3] += value;
    }

    void ComputeClusterDeformation(
        FbxAMatrix& outVertexTransform,
        FbxMesh* mesh,
        FbxCluster* cluster,
        const FbxAMatrix& meshGlobalCurrent,
        const FbxTime& evaluationTime) {

        outVertexTransform.SetIdentity();
        if (!mesh || !cluster || !cluster->GetLink()) {
            return;
        }

        FbxAMatrix referenceGlobalInitial;
        FbxAMatrix referenceGlobalCurrent = meshGlobalCurrent;
        FbxAMatrix clusterGlobalInitial;
        FbxAMatrix clusterGlobalCurrent =
            cluster->GetLink()->EvaluateGlobalTransform(
                evaluationTime);

        cluster->GetTransformMatrix(
            referenceGlobalInitial);
        referenceGlobalInitial *=
            GetFBXGeometryTransform(
                mesh->GetNode());

        cluster->GetTransformLinkMatrix(
            clusterGlobalInitial);

        if (cluster->GetLinkMode() == FbxCluster::eAdditive &&
            cluster->GetAssociateModel()) {

            FbxAMatrix associateGlobalInitial;
            FbxAMatrix associateGlobalCurrent =
                cluster->GetAssociateModel()->
                    EvaluateGlobalTransform(
                        evaluationTime);

            cluster->GetTransformAssociateModelMatrix(
                associateGlobalInitial);

            outVertexTransform =
                referenceGlobalInitial.Inverse() *
                associateGlobalInitial *
                associateGlobalCurrent.Inverse() *
                clusterGlobalCurrent *
                clusterGlobalInitial.Inverse() *
                referenceGlobalInitial;
            return;
        }

        const FbxAMatrix clusterRelativeInitial =
            clusterGlobalInitial.Inverse() *
            referenceGlobalInitial;

        const FbxAMatrix clusterRelativeCurrentInverse =
            referenceGlobalCurrent.Inverse() *
            clusterGlobalCurrent;

        outVertexTransform =
            clusterRelativeCurrentInverse *
            clusterRelativeInitial;
    }

    bool ApplyFBXBlendShapes(
        FbxMesh* mesh,
        std::vector<FbxVector4>& controlPoints) {

        if (!mesh || controlPoints.empty()) {
            return false;
        }

        const int blendShapeCount =
            mesh->GetDeformerCount(FbxDeformer::eBlendShape);
        if (blendShapeCount <= 0) {
            return false;
        }

        std::vector<FbxVector4> baseControlPoints = controlPoints;
        bool changed = false;

        for (int blendShapeIndex = 0;
            blendShapeIndex < blendShapeCount;
            ++blendShapeIndex) {

            FbxBlendShape* blendShape =
                static_cast<FbxBlendShape*>(
                    mesh->GetDeformer(
                        blendShapeIndex,
                        FbxDeformer::eBlendShape));
            if (!blendShape) {
                continue;
            }

            const int channelCount =
                blendShape->GetBlendShapeChannelCount();
            for (int channelIndex = 0;
                channelIndex < channelCount;
                ++channelIndex) {

                FbxBlendShapeChannel* channel =
                    blendShape->GetBlendShapeChannel(channelIndex);
                if (!channel) {
                    continue;
                }

                const int targetCount =
                    channel->GetTargetShapeCount();
                if (targetCount <= 0) {
                    continue;
                }

                const double deformPercent =
                    channel->DeformPercent.Get();
                if (!std::isfinite(deformPercent) ||
                    fabs(deformPercent) <= 0.000001) {
                    continue;
                }

                double* fullWeights =
                    channel->GetTargetShapeFullWeights();

                int upperTarget = targetCount - 1;
                for (int targetIndex = 0;
                    targetIndex < targetCount;
                    ++targetIndex) {
                    const double fullWeight =
                        fullWeights
                        ? fullWeights[targetIndex]
                        : 100.0;
                    if (deformPercent <= fullWeight) {
                        upperTarget = targetIndex;
                        break;
                    }
                }

                FbxShape* upperShape =
                    channel->GetTargetShape(upperTarget);
                if (!upperShape ||
                    upperShape->GetControlPointsCount() !=
                        static_cast<int>(baseControlPoints.size())) {
                    continue;
                }

                FbxShape* lowerShape = nullptr;
                double lowerWeight = 0.0;
                if (upperTarget > 0) {
                    lowerShape =
                        channel->GetTargetShape(upperTarget - 1);
                    lowerWeight =
                        fullWeights
                        ? fullWeights[upperTarget - 1]
                        : static_cast<double>(upperTarget) * 100.0;
                }

                const double upperWeight =
                    fullWeights
                    ? fullWeights[upperTarget]
                    : static_cast<double>(upperTarget + 1) * 100.0;

                double interpolation = 1.0;
                const double interval =
                    upperWeight - lowerWeight;
                if (fabs(interval) > 0.000001) {
                    interpolation =
                        (deformPercent - lowerWeight) / interval;
                }
                interpolation =
                    (std::max)(0.0, (std::min)(1.0, interpolation));

                for (size_t controlPointIndex = 0;
                    controlPointIndex < baseControlPoints.size();
                    ++controlPointIndex) {

                    const FbxVector4 lowerPoint =
                        lowerShape &&
                            lowerShape->GetControlPointsCount() ==
                                static_cast<int>(baseControlPoints.size())
                        ? lowerShape->GetControlPointAt(
                            static_cast<int>(controlPointIndex))
                        : baseControlPoints[controlPointIndex];

                    const FbxVector4 upperPoint =
                        upperShape->GetControlPointAt(
                            static_cast<int>(controlPointIndex));

                    const FbxVector4 channelPoint =
                        lowerPoint +
                        (upperPoint - lowerPoint) * interpolation;

                    controlPoints[controlPointIndex] +=
                        channelPoint - baseControlPoints[controlPointIndex];
                    controlPoints[controlPointIndex][3] = 1.0;
                }

                changed = true;
            }
        }

        return changed;
    }

    bool DeformFBXControlPoints(
        FbxMesh* mesh,
        FbxNode* node,
        const FbxTime& evaluationTime,
        std::vector<FbxVector4>& outControlPoints) {

        if (!mesh || !node) {
            return false;
        }

        const int controlPointCount =
            mesh->GetControlPointsCount();
        if (controlPointCount <= 0) {
            return false;
        }

        outControlPoints.resize(
            static_cast<size_t>(controlPointCount));
        for (int controlPointIndex = 0;
            controlPointIndex < controlPointCount;
            ++controlPointIndex) {
            outControlPoints[controlPointIndex] =
                mesh->GetControlPointAt(
                    controlPointIndex);
        }

        const bool hasBlendShape =
            ApplyFBXBlendShapes(mesh, outControlPoints);

        const int skinCount =
            mesh->GetDeformerCount(
                FbxDeformer::eSkin);
        if (skinCount <= 0) {
            return hasBlendShape;
        }

        std::vector<FbxAMatrix> deformationMatrices(
            static_cast<size_t>(controlPointCount));
        std::vector<double> deformationWeights(
            static_cast<size_t>(controlPointCount),
            0.0);

        for (FbxAMatrix& matrix : deformationMatrices) {
            MatrixSetZero(matrix);
        }

        const FbxAMatrix meshGlobalCurrent =
            node->EvaluateGlobalTransform(
                evaluationTime);

        FbxCluster::ELinkMode linkMode =
            FbxCluster::eNormalize;
        bool foundValidCluster = false;

        for (int skinIndex = 0;
            skinIndex < skinCount;
            ++skinIndex) {

            FbxSkin* skin =
                static_cast<FbxSkin*>(
                    mesh->GetDeformer(
                        skinIndex,
                        FbxDeformer::eSkin));
            if (!skin) {
                continue;
            }

            const int clusterCount =
                skin->GetClusterCount();
            for (int clusterIndex = 0;
                clusterIndex < clusterCount;
                ++clusterIndex) {

                FbxCluster* cluster =
                    skin->GetCluster(
                        clusterIndex);
                if (!cluster || !cluster->GetLink()) {
                    continue;
                }

                foundValidCluster = true;
                linkMode = cluster->GetLinkMode();

                FbxAMatrix vertexTransform;
                ComputeClusterDeformation(
                    vertexTransform,
                    mesh,
                    cluster,
                    meshGlobalCurrent,
                    evaluationTime);

                const int indexCount =
                    cluster->GetControlPointIndicesCount();
                const int* indices =
                    cluster->GetControlPointIndices();
                const double* weights =
                    cluster->GetControlPointWeights();

                for (int influenceIndex = 0;
                    influenceIndex < indexCount;
                    ++influenceIndex) {

                    const int controlPointIndex =
                        indices[influenceIndex];
                    const double weight =
                        weights[influenceIndex];

                    if (controlPointIndex < 0 ||
                        controlPointIndex >= controlPointCount ||
                        weight <= 0.0) {
                        continue;
                    }

                    FbxAMatrix influence =
                        vertexTransform;

                    if (linkMode == FbxCluster::eAdditive) {
                        MatrixScale(influence, weight);
                        MatrixAddToDiagonal(
                            influence,
                            1.0 - weight);

                        if (deformationWeights[controlPointIndex] == 0.0) {
                            deformationMatrices[controlPointIndex].
                                SetIdentity();
                        }

                        deformationMatrices[controlPointIndex] =
                            influence *
                            deformationMatrices[controlPointIndex];
                        deformationWeights[controlPointIndex] = 1.0;
                    }
                    else {
                        MatrixScale(influence, weight);
                        MatrixAdd(
                            deformationMatrices[controlPointIndex],
                            influence);
                        deformationWeights[controlPointIndex] +=
                            weight;
                    }
                }
            }
        }

        if (!foundValidCluster) {
            return hasBlendShape;
        }

        for (int controlPointIndex = 0;
            controlPointIndex < controlPointCount;
            ++controlPointIndex) {

            const double weight =
                deformationWeights[controlPointIndex];
            if (weight <= 0.0) {
                continue;
            }

            const FbxVector4 source =
                outControlPoints[
                    static_cast<size_t>(controlPointIndex)];
            FbxVector4 deformed =
                deformationMatrices[controlPointIndex].
                    MultT(source);

            if (linkMode == FbxCluster::eNormalize &&
                weight != 0.0) {
                deformed /= weight;
            }
            else if (linkMode == FbxCluster::eTotalOne) {
                deformed += source * (1.0 - weight);
            }

            deformed[3] = 1.0;
            outControlPoints[controlPointIndex] =
                deformed;
        }

        return true;
    }

}

bool Model3D::load(const std::string& path) {
    unload();
    SetPath(path);
    SetState(ResourceState::Loading);

    if (path.empty()) {
        ERROR("ModelLoader", "load", "La ruta del modelo esta vacia.");
        SetState(ResourceState::Failed);
        return false;
    }

    std::ifstream fileCheck(path.c_str(), std::ios::binary);
    if (!fileCheck.good()) {
        ERROR("ModelLoader", "load", "No se encontro el archivo del modelo: " << path.c_str());
        SetState(ResourceState::Failed);
        return false;
    }
    fileCheck.close();

    const bool success = init();
    SetState(success ? ResourceState::Loaded : ResourceState::Failed);
    return success;
}

bool Model3D::init() {
    m_meshes.clear();
    textureFileNames.clear();
    m_fbxMaterialPointers.clear();
    m_importedMaterials.clear();

    switch (m_modelType) {
    case FBX:
        LoadFBXModel(m_filePath);
        break;
    case OBJ:
        LoadOBJModel(m_filePath);
        break;
    default:
        ERROR("ModelLoader", "init", "Formato de modelo no soportado.");
        return false;
    }

    if (m_meshes.empty()) {
        ERROR("ModelLoader", "init", "El modelo no contiene mallas validas.");
        return false;
    }

    if (m_importedMaterials.empty()) {
        m_importedMaterials.push_back(
            ImportedMaterialInfo());
    }

    return true;
}

void Model3D::unload() {
    releaseFBXResources();
    m_meshes.clear();
    textureFileNames.clear();
    m_fbxMaterialPointers.clear();
    m_importedMaterials.clear();
    SetState(ResourceState::Unloaded);
}

size_t Model3D::getSizeInBytes() const {
    size_t total = 0;
    for (const MeshComponent& mesh : m_meshes) {
        total += mesh.m_vertex.size() * sizeof(SimpleVertex);
        total += mesh.m_skyVertex.size() * sizeof(SkyboxVertex);
        total += mesh.m_index.size() * sizeof(unsigned int);
    }
    return total;
}

void Model3D::releaseFBXResources() {
    lScene = nullptr;
    if (lSdkManager) {
        lSdkManager->Destroy();
        lSdkManager = nullptr;
    }
    m_fbxMaterialPointers.clear();
}

std::string Model3D::getFBXTexturePath(const FbxProperty& property) {
    if (!property.IsValid()) {
        return std::string();
    }

    const int layeredTextureCount =
        property.GetSrcObjectCount<FbxLayeredTexture>();

    for (int layeredIndex = 0;
        layeredIndex < layeredTextureCount;
        ++layeredIndex) {

        FbxLayeredTexture* layeredTexture =
            property.GetSrcObject<FbxLayeredTexture>(
                layeredIndex);

        if (!layeredTexture) {
            continue;
        }

        const int fileTextureCount =
            layeredTexture->
                GetSrcObjectCount<FbxFileTexture>();

        for (int textureIndex = 0;
            textureIndex < fileTextureCount;
            ++textureIndex) {

            FbxFileTexture* fileTexture =
                layeredTexture->
                    GetSrcObject<FbxFileTexture>(
                        textureIndex);

            if (!fileTexture) {
                continue;
            }

            const char* relativePath =
                fileTexture->GetRelativeFileName();
            const char* absolutePath =
                fileTexture->GetFileName();

            if (relativePath && relativePath[0] != '\0') {
                return relativePath;
            }
            if (absolutePath && absolutePath[0] != '\0') {
                return absolutePath;
            }
        }
    }

    const int textureCount =
        property.GetSrcObjectCount<FbxFileTexture>();

    for (int textureIndex = 0;
        textureIndex < textureCount;
        ++textureIndex) {

        FbxFileTexture* fileTexture =
            property.GetSrcObject<FbxFileTexture>(
                textureIndex);

        if (!fileTexture) {
            continue;
        }

        const char* relativePath =
            fileTexture->GetRelativeFileName();
        const char* absolutePath =
            fileTexture->GetFileName();

        if (relativePath && relativePath[0] != '\0') {
            return relativePath;
        }
        if (absolutePath && absolutePath[0] != '\0') {
            return absolutePath;
        }
    }

    return std::string();
}

std::string Model3D::getFBXTexturePathByPropertyName(
    FbxSurfaceMaterial* material,
    const char* propertyName) {

    if (!material || !propertyName) {
        return std::string();
    }

    return getFBXTexturePath(
        material->FindProperty(propertyName));
}

ImportedMaterialInfo Model3D::extractFBXMaterial(
    FbxSurfaceMaterial* material) const {

    ImportedMaterialInfo info;

    if (!material) {
        return info;
    }

    if (material->GetName() &&
        material->GetName()[0] != '\0') {
        info.name = material->GetName();
    }

    FbxProperty diffuseProperty =
        material->FindProperty(
            FbxSurfaceMaterial::sDiffuse);

    if (diffuseProperty.IsValid()) {
        const FbxDouble3 diffuse =
            diffuseProperty.Get<FbxDouble3>();

        info.baseColor.x =
            static_cast<float>(diffuse[0]);
        info.baseColor.y =
            static_cast<float>(diffuse[1]);
        info.baseColor.z =
            static_cast<float>(diffuse[2]);

        info.albedoTexture =
            getFBXTexturePath(
                diffuseProperty);
    }

    info.normalTexture =
        getFBXTexturePathByPropertyName(
            material,
            FbxSurfaceMaterial::sNormalMap);

    if (info.normalTexture.empty()) {
        info.normalTexture =
            getFBXTexturePathByPropertyName(
                material,
                FbxSurfaceMaterial::sBump);
    }

    info.emissiveTexture =
        getFBXTexturePathByPropertyName(
            material,
            FbxSurfaceMaterial::sEmissive);

    FbxProperty transparencyProperty =
        material->FindProperty(
            FbxSurfaceMaterial::sTransparencyFactor);

    if (transparencyProperty.IsValid()) {
        const double transparency =
            transparencyProperty.Get<FbxDouble>();

        info.opacity =
            static_cast<float>(
                1.0 -
                (std::max)(
                    0.0,
                    (std::min)(
                        1.0,
                        transparency)));
    }

    FbxSurfacePhong* phongMaterial =
        FbxCast<FbxSurfacePhong>(
            material);

    if (phongMaterial) {
        const double shininess =
            (std::max)(
                0.0,
                phongMaterial->
                    Shininess.Get());

        info.roughness =
            static_cast<float>(
                std::sqrt(
                    2.0 /
                    (shininess + 2.0)));
    }

    auto lowerCopy =
        [](std::string value) {
            for (char& character : value) {
                if (character >= 'A' &&
                    character <= 'Z') {
                    character =
                        static_cast<char>(
                            character +
                            ('a' - 'A'));
                }
            }
            return value;
        };

    for (FbxProperty property =
            material->GetFirstProperty();
        property.IsValid();
        property =
            material->GetNextProperty(
                property)) {

        const std::string propertyName =
            lowerCopy(
                property.GetNameAsCStr());

        const std::string texturePath =
            getFBXTexturePath(
                property);

        if (!texturePath.empty()) {
            if (info.albedoTexture.empty() &&
                (propertyName.find("basecolor") != std::string::npos ||
                 propertyName.find("base_color") != std::string::npos ||
                 propertyName.find("diffuse") != std::string::npos ||
                 propertyName.find("albedo") != std::string::npos)) {
                info.albedoTexture = texturePath;
            }
            else if (info.normalTexture.empty() &&
                (propertyName.find("normal") != std::string::npos ||
                 propertyName.find("bump") != std::string::npos)) {
                info.normalTexture = texturePath;
            }
            else if (info.metallicTexture.empty() &&
                (propertyName.find("metallic") != std::string::npos ||
                 propertyName.find("metalness") != std::string::npos)) {
                info.metallicTexture = texturePath;
            }
            else if (info.roughnessTexture.empty() &&
                propertyName.find("rough") != std::string::npos) {
                info.roughnessTexture = texturePath;
            }
            else if (info.aoTexture.empty() &&
                (propertyName == "ao" ||
                 propertyName.find("occlusion") != std::string::npos ||
                 propertyName.find("ambientocclusion") != std::string::npos)) {
                info.aoTexture = texturePath;
            }
            else if (info.emissiveTexture.empty() &&
                (propertyName.find("emissive") != std::string::npos ||
                 propertyName.find("emission") != std::string::npos)) {
                info.emissiveTexture = texturePath;
            }
        }

        if (propertyName.find("metallic") != std::string::npos ||
            propertyName.find("metalness") != std::string::npos) {
            const EFbxType propertyType =
                property.GetPropertyDataType().GetType();

            if (propertyType == eFbxDouble) {
                info.metallic =
                    static_cast<float>(
                        property.Get<FbxDouble>());
            }
            else if (propertyType == eFbxFloat) {
                info.metallic =
                    property.Get<FbxFloat>();
            }
        }
        else if (propertyName.find("roughness") !=
            std::string::npos) {
            const EFbxType propertyType =
                property.GetPropertyDataType().GetType();

            if (propertyType == eFbxDouble) {
                info.roughness =
                    static_cast<float>(
                        property.Get<FbxDouble>());
            }
            else if (propertyType == eFbxFloat) {
                info.roughness =
                    property.Get<FbxFloat>();
            }
        }
        else if (propertyName == "opacity") {
            const EFbxType propertyType =
                property.GetPropertyDataType().GetType();

            if (propertyType == eFbxDouble) {
                info.opacity =
                    static_cast<float>(
                        property.Get<FbxDouble>());
            }
            else if (propertyType == eFbxFloat) {
                info.opacity =
                    property.Get<FbxFloat>();
            }
        }
    }

    info.metallic =
        (std::max)(
            0.0f,
            (std::min)(
                1.0f,
                info.metallic));

    info.roughness =
        (std::max)(
            0.02f,
            (std::min)(
                1.0f,
                info.roughness));

    info.opacity =
        (std::max)(
            0.0f,
            (std::min)(
                1.0f,
                info.opacity));

    info.baseColor.w =
        info.opacity;

    return info;
}

unsigned int Model3D::registerFBXMaterial(
    FbxSurfaceMaterial* material) {

    for (size_t materialIndex = 0;
        materialIndex <
            m_fbxMaterialPointers.size();
        ++materialIndex) {

        if (m_fbxMaterialPointers[
                materialIndex] ==
            material) {
            return static_cast<unsigned int>(
                materialIndex);
        }
    }

    m_fbxMaterialPointers.push_back(
        material);

    m_importedMaterials.push_back(
        extractFBXMaterial(
            material));

    return static_cast<unsigned int>(
        m_importedMaterials.size() - 1);
}

bool Model3D::InitializeFBXManager() {
    releaseFBXResources();

    lSdkManager = FbxManager::Create();
    if (!lSdkManager) {
        ERROR("ModelLoader", "FbxManager::Create", "Unable to create FBX Manager.");
        return false;
    }

    FbxIOSettings* ioSettings = FbxIOSettings::Create(lSdkManager, IOSROOT);
    if (!ioSettings) {
        ERROR("ModelLoader", "FbxIOSettings::Create", "Unable to create FBX IO settings.");
        releaseFBXResources();
        return false;
    }
    lSdkManager->SetIOSettings(ioSettings);

    lScene = FbxScene::Create(lSdkManager, "WildVineImportedScene");
    if (!lScene) {
        ERROR("ModelLoader", "FbxScene::Create", "Unable to create FBX Scene.");
        releaseFBXResources();
        return false;
    }

    return true;
}

std::vector<MeshComponent> Model3D::LoadFBXModel(const std::string& filePath) {
    m_meshes.clear();

    if (!InitializeFBXManager()) {
        return m_meshes;
    }

    if (lSdkManager->GetIOSettings()) {
        lSdkManager->GetIOSettings()->SetBoolProp(
            IMP_FBX_MATERIAL,
            true);
        lSdkManager->GetIOSettings()->SetBoolProp(
            IMP_FBX_TEXTURE,
            true);
        lSdkManager->GetIOSettings()->SetBoolProp(
            IMP_FBX_EXTRACT_EMBEDDED_DATA,
            true);
    }

    FbxImporter* importer = FbxImporter::Create(lSdkManager, "");
    if (!importer) {
        ERROR("ModelLoader", "FbxImporter::Create", "Unable to create FBX Importer.");
        releaseFBXResources();
        return m_meshes;
    }

    if (!importer->Initialize(filePath.c_str(), -1, lSdkManager->GetIOSettings())) {
        ERROR("ModelLoader", "FbxImporter::Initialize",
            "Unable to initialize FBX Importer: " << importer->GetStatus().GetErrorString());
        importer->Destroy();
        releaseFBXResources();
        return m_meshes;
    }

    if (!importer->Import(lScene)) {
        ERROR("ModelLoader", "FbxImporter::Import",
            "Unable to import FBX Scene: " << importer->GetStatus().GetErrorString());
        importer->Destroy();
        releaseFBXResources();
        return m_meshes;
    }

    m_name = importer->GetFileName();
    importer->Destroy();

    // IMPORTANTE:
    // No se modifica la escena con ConvertScene/ConvertSystemUnit antes de
    // evaluar sus nodos. Algunos exportadores guardan PreRotation y pivotes
    // que, al convertir la escena completa, pueden terminar aplicandose dos
    // veces. En su lugar se conserva la escena original y se calcula una
    // matriz de conversion independiente para transformar el resultado final.
    FbxAxisSystem sourceAxis =
        lScene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem targetAxis = FbxAxisSystem::DirectX;

    m_sourceToEngineAxis.SetIdentity();

    // GetMatrix() representa la base completa de cada sistema, incluyendo
    // handedness. La conversion de coordenadas se obtiene sin modificar la
    // escena: vEngine = inverse(TargetBasis) * SourceBasis * vSource.
    // Esto es importante porque ConvertScene() solo rota nodos raiz y no
    // puede representar cambios de handedness.
    FbxAMatrix sourceAxisMatrix;
    FbxAMatrix targetAxisMatrix;
    sourceAxisMatrix.SetIdentity();
    targetAxisMatrix.SetIdentity();
    sourceAxis.GetMatrix(sourceAxisMatrix);
    targetAxis.GetMatrix(targetAxisMatrix);
    m_sourceToEngineAxis =
        targetAxisMatrix.Inverse() * sourceAxisMatrix;

    const FbxSystemUnit sourceUnit =
        lScene->GetGlobalSettings().GetSystemUnit();
    m_sourceToMeters =
        sourceUnit.GetConversionFactorTo(FbxSystemUnit::m);

    if (!std::isfinite(m_sourceToMeters) ||
        m_sourceToMeters <= 0.0) {
        m_sourceToMeters = 1.0;
    }

    MESSAGE(
        "ModelLoader",
        "LoadFBXModel",
        "Conversion FBX preparada. Factor a metros: "
            << m_sourceToMeters);

    FbxGeometryConverter geometryConverter(lSdkManager);
    geometryConverter.Triangulate(lScene, true);

    FbxNode* rootNode = lScene->GetRootNode();
    if (!rootNode) {
        ERROR("ModelLoader", "GetRootNode", "Unable to get root node from FBX Scene.");
        releaseFBXResources();
        return m_meshes;
    }

    for (int childIndex = 0;
        childIndex < rootNode->GetChildCount();
        ++childIndex) {
        ProcessFBXNode(rootNode->GetChild(childIndex));
    }

    releaseFBXResources();
    return m_meshes;
}

std::vector<MeshComponent> Model3D::LoadOBJModel(const std::string& filePath) {
    m_meshes.clear();

    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        ERROR("ModelLoader", "LoadOBJModel", "No se pudo abrir el archivo OBJ.");
        return m_meshes;
    }

    std::vector<EU::Vector3> positions;
    std::vector<EU::Vector2> texcoords;
    std::vector<EU::Vector3> normals;

    std::unordered_map<std::string, unsigned int> objMaterialSlots;
    unsigned int currentMaterialSlot = 0;
    std::string currentObjectName = "OBJMesh";
    std::string currentMaterialName = "Default";

    auto ensureMaterialSlot =
        [&](const std::string& requestedName) {
            const std::string materialName =
                requestedName.empty()
                ? std::string("Default")
                : requestedName;

            const auto existing =
                objMaterialSlots.find(materialName);
            if (existing != objMaterialSlots.end()) {
                return existing->second;
            }

            ImportedMaterialInfo material;
            material.name = materialName;
            const unsigned int slot =
                static_cast<unsigned int>(m_importedMaterials.size());
            m_importedMaterials.push_back(material);
            objMaterialSlots[materialName] = slot;
            return slot;
        };

    currentMaterialSlot = ensureMaterialSlot(currentMaterialName);

    MeshComponent currentMesh;
    currentMesh.m_name = currentObjectName;
    currentMesh.m_materialSlot = currentMaterialSlot;
    std::unordered_map<std::string, unsigned int> vertexLookup;

    auto resetCurrentMesh = [&]() {
        currentMesh = MeshComponent();
        currentMesh.m_name =
            currentMaterialName == "Default"
            ? currentObjectName
            : currentObjectName + "__mat_" + currentMaterialName;
        currentMesh.m_materialSlot = currentMaterialSlot;
        vertexLookup.clear();
    };

    auto flushCurrentMesh = [&]() {
        if (currentMesh.m_vertex.empty() ||
            currentMesh.m_index.empty()) {
            resetCurrentMesh();
            return;
        }

        FinalizeObjMesh(currentMesh);
        m_meshes.push_back(currentMesh);
        resetCurrentMesh();
    };

    const std::string objDirectory = DirectoryOfFile(filePath);

    std::string line;
    size_t lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        line = TrimText(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream stream(line);
        std::string command;
        stream >> command;
        if (command.empty()) {
            continue;
        }

        if (command == "v") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (stream >> x >> y >> z) {
                positions.push_back(EU::Vector3(x, y, z));
            }
        }
        else if (command == "vt") {
            float u = 0.0f;
            float v = 0.0f;
            if (stream >> u >> v) {
                texcoords.push_back(EU::Vector2(u, 1.0f - v));
            }
        }
        else if (command == "vn") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (stream >> x >> y >> z) {
                normals.push_back(
                    EU::Vector3(x, y, z).normalize());
            }
        }
        else if (command == "o" || command == "g") {
            std::string name;
            std::getline(stream, name);
            name = TrimText(name);

            flushCurrentMesh();
            if (!name.empty()) {
                currentObjectName = name;
            }
            resetCurrentMesh();
        }
        else if (command == "mtllib") {
            std::string libraryReference;
            std::getline(stream, libraryReference);
            libraryReference = TrimText(libraryReference);

            if (libraryReference.empty()) {
                continue;
            }

            // OBJ permite mas de una biblioteca en la misma linea. Primero se
            // intenta la linea completa (soporta espacios en el nombre) y, si
            // no existe, se prueban los tokens individuales.
            const std::string completePath =
                JoinFilePath(objDirectory, libraryReference);

            bool loadedLibrary = LoadMtlLibrary(
                completePath,
                m_importedMaterials,
                objMaterialSlots);

            if (!loadedLibrary) {
                std::istringstream libraries(libraryReference);
                std::string libraryName;
                while (libraries >> libraryName) {
                    const std::string libraryPath =
                        JoinFilePath(objDirectory, libraryName);
                    if (LoadMtlLibrary(
                        libraryPath,
                        m_importedMaterials,
                        objMaterialSlots)) {
                        loadedLibrary = true;
                    }
                }
            }

            if (!loadedLibrary) {
                ERROR(
                    "ModelLoader",
                    "LoadOBJModel",
                    "No se pudo abrir la biblioteca MTL: "
                        << libraryReference.c_str());
            }
        }
        else if (command == "usemtl") {
            std::string materialName;
            std::getline(stream, materialName);
            materialName = TrimText(materialName);
            if (materialName.empty()) {
                materialName = "Default";
            }

            flushCurrentMesh();
            currentMaterialName = materialName;
            currentMaterialSlot =
                ensureMaterialSlot(currentMaterialName);
            resetCurrentMesh();
        }
        else if (command == "f") {
            std::vector<unsigned int> faceIndices;
            std::string token;
            bool validFace = true;

            while (stream >> token) {
                const auto existing = vertexLookup.find(token);
                if (existing != vertexLookup.end()) {
                    faceIndices.push_back(existing->second);
                    continue;
                }

                ObjIndex parsed;
                if (!ParseObjIndex(token, parsed)) {
                    validFace = false;
                    break;
                }

                const int positionIndex =
                    ResolveObjIndex(parsed.position, positions.size());
                const int texcoordIndex =
                    ResolveObjIndex(parsed.texcoord, texcoords.size());
                const int normalIndex =
                    ResolveObjIndex(parsed.normal, normals.size());

                if (positionIndex < 0 ||
                    positionIndex >= static_cast<int>(positions.size())) {
                    validFace = false;
                    break;
                }

                SimpleVertex vertex{};
                vertex.Position = positions[positionIndex];
                vertex.TextureCoordinate =
                    texcoordIndex >= 0 &&
                        texcoordIndex < static_cast<int>(texcoords.size())
                    ? texcoords[texcoordIndex]
                    : EU::Vector2(0.0f, 0.0f);
                vertex.Normal =
                    normalIndex >= 0 &&
                        normalIndex < static_cast<int>(normals.size())
                    ? normals[normalIndex]
                    : EU::Vector3();
                vertex.Tangent = EU::Vector3();
                vertex.Bitangent = EU::Vector3();

                const unsigned int newIndex =
                    static_cast<unsigned int>(
                        currentMesh.m_vertex.size());
                currentMesh.m_vertex.push_back(vertex);
                vertexLookup[token] = newIndex;
                faceIndices.push_back(newIndex);
            }

            if (!validFace || faceIndices.size() < 3) {
                ERROR(
                    "ModelLoader",
                    "LoadOBJModel",
                    "Cara OBJ invalida en la linea " << lineNumber);
                continue;
            }

            for (size_t triangleIndex = 1;
                triangleIndex + 1 < faceIndices.size();
                ++triangleIndex) {
                currentMesh.m_index.push_back(faceIndices[0]);
                currentMesh.m_index.push_back(
                    faceIndices[triangleIndex + 1]);
                currentMesh.m_index.push_back(
                    faceIndices[triangleIndex]);
            }
        }
    }

    flushCurrentMesh();

    if (m_meshes.empty()) {
        ERROR(
            "ModelLoader",
            "LoadOBJModel",
            "El OBJ no contiene geometria util.");
    }
    else {
        MESSAGE(
            "ModelLoader",
            "LoadOBJModel",
            "OBJ importado correctamente. Mallas: "
                << m_meshes.size()
                << " | Materiales: "
                << m_importedMaterials.size());
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

    // La transformacion del nodo se evalua en el sistema ORIGINAL del FBX.
    // Despues se convierte el resultado a DirectX y a metros exactamente una
    // vez. Esto conserva PreRotation, PostRotation, pivotes, escalas y toda la
    // jerarquia sin pedirle al SDK que reescriba previamente la escena.
    const FbxTime evaluationTime = FBXSDK_TIME_ZERO;
    const FbxAMatrix geometricTransform =
        GetFBXGeometryTransform(node);
    const FbxAMatrix sourceNodeGlobal =
        node->EvaluateGlobalTransform(
            evaluationTime,
            FbxNode::eSourcePivot,
            false,
            true);

    FbxAMatrix unitTransform;
    unitTransform.SetIdentity();
    unitTransform.SetS(FbxVector4(
        m_sourceToMeters,
        m_sourceToMeters,
        m_sourceToMeters,
        1.0));

    const FbxAMatrix vertexTransform =
        m_sourceToEngineAxis *
        unitTransform *
        sourceNodeGlobal *
        geometricTransform;

    const FbxAMatrix normalTransform =
        vertexTransform.Inverse().Transpose();

    // GetS() puede perder el signo de una reflexion. El determinante del
    // bloque lineal permite corregir el winding de forma fiable al convertir
    // entre sistemas diestros y zurdos o al importar escalas negativas.
    const double linearDeterminant =
        vertexTransform[0][0] *
            (vertexTransform[1][1] * vertexTransform[2][2] -
             vertexTransform[1][2] * vertexTransform[2][1]) -
        vertexTransform[0][1] *
            (vertexTransform[1][0] * vertexTransform[2][2] -
             vertexTransform[1][2] * vertexTransform[2][0]) +
        vertexTransform[0][2] *
            (vertexTransform[1][0] * vertexTransform[2][1] -
             vertexTransform[1][1] * vertexTransform[2][0]);

    const bool mirroredTransform =
        std::isfinite(linearDeterminant) &&
        linearDeterminant < 0.0;

    // Los FBX con Skin o Blend Shapes no pueden reconstruirse leyendo solo
    // los control points base. Se evalua su pose estatica de importacion para
    // conservar la apariencia que el archivo guarda en el tiempo cero.
    std::vector<FbxVector4> evaluatedControlPoints;
    const bool hasEvaluatedSkin =
        DeformFBXControlPoints(
            mesh,
            node,
            evaluationTime,
            evaluatedControlPoints);

    if (hasEvaluatedSkin) {
        MESSAGE(
            "ModelLoader",
            "ProcessFBXMesh",
            "Deformacion estatica evaluada: "
                << (node->GetName()
                    ? node->GetName()
                    : "Mesh"));
    }

    if (mesh->GetElementNormalCount() == 0) {
        mesh->GenerateNormals(true, true);
    }

    FbxStringList uvSets;
    mesh->GetUVSetNames(uvSets);
    const char* uvSetName =
        uvSets.GetCount() > 0
        ? uvSets[0]
        : nullptr;

    if (mesh->GetElementTangentCount() == 0 && uvSetName) {
        mesh->GenerateTangentsData(uvSetName);
    }

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

    const std::string nodeName =
        node->GetName() && node->GetName()[0] != '\0'
        ? node->GetName()
        : "Mesh";

    int materialCount = node->GetMaterialCount();
    if (materialCount < 1) {
        materialCount = 1;
    }

    struct MeshBuilder {
        std::string name;
        unsigned int materialSlot = 0;
        std::vector<SimpleVertex> vertices;
        std::vector<unsigned int> indices;
    };

    std::vector<MeshBuilder> builders(
        static_cast<size_t>(materialCount));

    for (int materialIndex = 0;
        materialIndex < materialCount;
        ++materialIndex) {

        FbxSurfaceMaterial* material =
            materialIndex < node->GetMaterialCount()
            ? node->GetMaterial(materialIndex)
            : nullptr;

        const std::string materialName =
            material && material->GetName() &&
                material->GetName()[0] != '\0'
            ? material->GetName()
            : "Default";

        builders[materialIndex].name =
            nodeName + "__mat_" + materialName;
        builders[materialIndex].materialSlot =
            registerFBXMaterial(material);
    }

    auto getPolygonMaterialIndex =
        [materialElement, materialCount](int polygonIndex) {
            if (!materialElement || materialCount <= 1) {
                return 0;
            }

            int materialIndex = 0;
            const FbxGeometryElement::EMappingMode mappingMode =
                materialElement->GetMappingMode();

            if (mappingMode == FbxGeometryElement::eByPolygon) {
                const int indexCount =
                    materialElement->GetIndexArray().GetCount();

                if (polygonIndex >= 0 && polygonIndex < indexCount) {
                    materialIndex =
                        materialElement->GetIndexArray().GetAt(
                            polygonIndex);
                }
            }
            else if (mappingMode == FbxGeometryElement::eAllSame) {
                if (materialElement->GetIndexArray().GetCount() > 0) {
                    materialIndex =
                        materialElement->GetIndexArray().GetAt(0);
                }
            }

            if (materialIndex < 0 || materialIndex >= materialCount) {
                materialIndex = 0;
            }

            return materialIndex;
        };

    auto readVector4 =
        [](const auto* element,
            int controlPointIndex,
            int polygonVertexIndex) {

            if (!element) {
                return FbxVector4(0.0, 0.0, 0.0, 0.0);
            }

            int elementIndex =
                element->GetMappingMode() ==
                    FbxGeometryElement::eByControlPoint
                ? controlPointIndex
                : polygonVertexIndex;

            if (elementIndex < 0) {
                return FbxVector4(0.0, 0.0, 0.0, 0.0);
            }

            if (element->GetReferenceMode() ==
                FbxGeometryElement::eIndexToDirect) {

                if (elementIndex >=
                    element->GetIndexArray().GetCount()) {
                    return FbxVector4(0.0, 0.0, 0.0, 0.0);
                }

                elementIndex =
                    element->GetIndexArray().GetAt(elementIndex);
            }

            if (elementIndex < 0 ||
                elementIndex >=
                    element->GetDirectArray().GetCount()) {
                return FbxVector4(0.0, 0.0, 0.0, 0.0);
            }

            return element->GetDirectArray().GetAt(elementIndex);
        };

    for (int polygonIndex = 0;
        polygonIndex < mesh->GetPolygonCount();
        ++polygonIndex) {

        const int polygonSize =
            mesh->GetPolygonSize(polygonIndex);

        if (polygonSize < 3) {
            continue;
        }

        const int materialIndex =
            getPolygonMaterialIndex(polygonIndex);
        MeshBuilder& builder =
            builders[static_cast<size_t>(materialIndex)];

        std::vector<unsigned int> polygonCorners;
        polygonCorners.reserve(static_cast<size_t>(polygonSize));

        for (int polygonCorner = 0;
            polygonCorner < polygonSize;
            ++polygonCorner) {

            const int controlPointIndex =
                mesh->GetPolygonVertex(
                    polygonIndex,
                    polygonCorner);

            if (controlPointIndex < 0 ||
                controlPointIndex >= mesh->GetControlPointsCount()) {
                continue;
            }

            const int polygonVertexIndex =
                mesh->GetPolygonVertexIndex(polygonIndex) +
                polygonCorner;

            SimpleVertex vertex{};

            const FbxVector4 localPosition =
                hasEvaluatedSkin &&
                    controlPointIndex <
                        static_cast<int>(
                            evaluatedControlPoints.size())
                ? evaluatedControlPoints[
                    static_cast<size_t>(controlPointIndex)]
                : mesh->GetControlPointAt(
                    controlPointIndex);
            const FbxVector4 position =
                vertexTransform.MultT(
                    localPosition);
            vertex.Position = EU::Vector3(
                static_cast<float>(position[0]),
                static_cast<float>(position[1]),
                static_cast<float>(position[2]));

            if (!hasEvaluatedSkin) {
                FbxVector4 localNormal(
                    0.0, 1.0, 0.0, 0.0);
                mesh->GetPolygonVertexNormal(
                    polygonIndex,
                    polygonCorner,
                    localNormal);
                localNormal[3] = 0.0;

                FbxVector4 normal =
                    normalTransform.MultT(
                        localNormal);
                normal[3] = 0.0;
                normal.Normalize();

                vertex.Normal = EU::Vector3(
                    static_cast<float>(normal[0]),
                    static_cast<float>(normal[1]),
                    static_cast<float>(normal[2]));
            }
            else {
                // En una malla deformada, la normal original ya no coincide
                // necesariamente con la pose. Se recalcula por triangulos.
                vertex.Normal = EU::Vector3(
                    0.0f, 0.0f, 0.0f);
            }

            FbxVector2 uv(0.0, 0.0);
            if (uvSetName) {
                bool unmapped = false;
                mesh->GetPolygonVertexUV(
                    polygonIndex,
                    polygonCorner,
                    uvSetName,
                    uv,
                    unmapped);

                if (unmapped) {
                    uv = FbxVector2(0.0, 0.0);
                }
            }

            vertex.TextureCoordinate = EU::Vector2(
                static_cast<float>(uv[0]),
                1.0f - static_cast<float>(uv[1]));

            if (!hasEvaluatedSkin) {
                FbxVector4 localTangent = readVector4(
                    tangentElement,
                    controlPointIndex,
                    polygonVertexIndex);
                localTangent[3] = 0.0;
                FbxVector4 tangent =
                    normalTransform.MultT(
                        localTangent);
                tangent[3] = 0.0;
                tangent.Normalize();
                vertex.Tangent = EU::Vector3(
                    static_cast<float>(tangent[0]),
                    static_cast<float>(tangent[1]),
                    static_cast<float>(tangent[2]));

                FbxVector4 localBitangent = readVector4(
                    binormalElement,
                    controlPointIndex,
                    polygonVertexIndex);
                localBitangent[3] = 0.0;
                FbxVector4 bitangent =
                    normalTransform.MultT(
                        localBitangent);
                bitangent[3] = 0.0;
                bitangent.Normalize();
                vertex.Bitangent = EU::Vector3(
                    static_cast<float>(bitangent[0]),
                    static_cast<float>(bitangent[1]),
                    static_cast<float>(bitangent[2]));
            }
            else {
                vertex.Tangent = EU::Vector3(
                    0.0f, 0.0f, 0.0f);
                vertex.Bitangent = EU::Vector3(
                    0.0f, 0.0f, 0.0f);
            }

            polygonCorners.push_back(
                static_cast<unsigned int>(builder.vertices.size()));
            builder.vertices.push_back(vertex);
        }

        if (polygonCorners.size() < 3) {
            continue;
        }

        // Mismo winding que el importador que funciona en ProyectoMiner.
        for (size_t triangleIndex = 1;
            triangleIndex + 1 < polygonCorners.size();
            ++triangleIndex) {

            builder.indices.push_back(polygonCorners[0]);
            builder.indices.push_back(
                polygonCorners[triangleIndex + 1]);
            builder.indices.push_back(
                polygonCorners[triangleIndex]);
        }
    }

    if (mirroredTransform) {
        for (MeshBuilder& builder : builders) {
            for (size_t index = 0;
                index + 2 < builder.indices.size();
                index += 3) {
                std::swap(
                    builder.indices[index + 1],
                    builder.indices[index + 2]);
            }
        }
    }

    auto safeNormalize =
        [](const EU::Vector3& value,
            const EU::Vector3& fallback) {
            const float lengthSquared =
                value.x * value.x +
                value.y * value.y +
                value.z * value.z;

            if (!std::isfinite(lengthSquared) ||
                lengthSquared <= 1e-12f) {
                return fallback;
            }

            const float inverseLength =
                1.0f / sqrtf(lengthSquared);
            return EU::Vector3(
                value.x * inverseLength,
                value.y * inverseLength,
                value.z * inverseLength);
        };

    for (MeshBuilder& builder : builders) {
        if (builder.vertices.empty() || builder.indices.empty()) {
            continue;
        }

        std::vector<EU::Vector3> accumulatedNormals(
            builder.vertices.size(),
            EU::Vector3(0.0f, 0.0f, 0.0f));
        std::vector<EU::Vector3> accumulatedTangents(
            builder.vertices.size(),
            EU::Vector3(0.0f, 0.0f, 0.0f));
        std::vector<EU::Vector3> accumulatedBitangents(
            builder.vertices.size(),
            EU::Vector3(0.0f, 0.0f, 0.0f));

        for (size_t index = 0;
            index + 2 < builder.indices.size();
            index += 3) {

            const unsigned int index0 = builder.indices[index + 0];
            const unsigned int index1 = builder.indices[index + 1];
            const unsigned int index2 = builder.indices[index + 2];

            if (index0 >= builder.vertices.size() ||
                index1 >= builder.vertices.size() ||
                index2 >= builder.vertices.size()) {
                continue;
            }

            const SimpleVertex& vertex0 = builder.vertices[index0];
            const SimpleVertex& vertex1 = builder.vertices[index1];
            const SimpleVertex& vertex2 = builder.vertices[index2];

            const EU::Vector3 edge1 =
                vertex1.Position - vertex0.Position;
            const EU::Vector3 edge2 =
                vertex2.Position - vertex0.Position;

            const EU::Vector3 faceNormal =
                EU::Vector3::cross(
                    edge1,
                    edge2).normalize();
            accumulatedNormals[index0] += faceNormal;
            accumulatedNormals[index1] += faceNormal;
            accumulatedNormals[index2] += faceNormal;

            const float deltaU1 =
                vertex1.TextureCoordinate.x -
                vertex0.TextureCoordinate.x;
            const float deltaV1 =
                vertex1.TextureCoordinate.y -
                vertex0.TextureCoordinate.y;
            const float deltaU2 =
                vertex2.TextureCoordinate.x -
                vertex0.TextureCoordinate.x;
            const float deltaV2 =
                vertex2.TextureCoordinate.y -
                vertex0.TextureCoordinate.y;

            const float denominator =
                deltaU1 * deltaV2 -
                deltaU2 * deltaV1;

            if (fabsf(denominator) <= 1e-8f) {
                continue;
            }

            const float inverse = 1.0f / denominator;
            const EU::Vector3 tangent(
                (edge1.x * deltaV2 - edge2.x * deltaV1) * inverse,
                (edge1.y * deltaV2 - edge2.y * deltaV1) * inverse,
                (edge1.z * deltaV2 - edge2.z * deltaV1) * inverse);
            const EU::Vector3 bitangent(
                (edge2.x * deltaU1 - edge1.x * deltaU2) * inverse,
                (edge2.y * deltaU1 - edge1.y * deltaU2) * inverse,
                (edge2.z * deltaU1 - edge1.z * deltaU2) * inverse);

            accumulatedTangents[index0] += tangent;
            accumulatedTangents[index1] += tangent;
            accumulatedTangents[index2] += tangent;
            accumulatedBitangents[index0] += bitangent;
            accumulatedBitangents[index1] += bitangent;
            accumulatedBitangents[index2] += bitangent;
        }

        for (size_t vertexIndex = 0;
            vertexIndex < builder.vertices.size();
            ++vertexIndex) {

            SimpleVertex& vertex = builder.vertices[vertexIndex];
            const EU::Vector3 sourceNormal =
                hasEvaluatedSkin ||
                    vertex.Normal.isNearlyZero()
                ? accumulatedNormals[vertexIndex]
                : vertex.Normal;

            vertex.Normal = safeNormalize(
                sourceNormal,
                EU::Vector3(0.0f, 1.0f, 0.0f));

            EU::Vector3 tangent =
                vertex.Tangent.isNearlyZero()
                ? accumulatedTangents[vertexIndex]
                : vertex.Tangent;

            const float normalProjection =
                EU::Vector3::dot(tangent, vertex.Normal);
            tangent -= vertex.Normal * normalProjection;

            if (tangent.isNearlyZero()) {
                const EU::Vector3 reference =
                    fabsf(vertex.Normal.y) < 0.99f
                    ? EU::Vector3(0.0f, 1.0f, 0.0f)
                    : EU::Vector3(1.0f, 0.0f, 0.0f);
                tangent = EU::Vector3::cross(
                    reference,
                    vertex.Normal);
            }

            vertex.Tangent = safeNormalize(
                tangent,
                EU::Vector3(1.0f, 0.0f, 0.0f));

            EU::Vector3 sourceBitangent =
                vertex.Bitangent.isNearlyZero()
                ? accumulatedBitangents[vertexIndex]
                : vertex.Bitangent;

            EU::Vector3 calculatedBitangent =
                EU::Vector3::cross(
                    vertex.Normal,
                    vertex.Tangent);

            const float handedness =
                !sourceBitangent.isNearlyZero() &&
                EU::Vector3::dot(
                    calculatedBitangent,
                    sourceBitangent) < 0.0f
                ? -1.0f
                : 1.0f;

            vertex.Bitangent = safeNormalize(
                calculatedBitangent * handedness,
                EU::Vector3(0.0f, 0.0f, 1.0f));
        }

        MeshComponent meshComponent;
        meshComponent.m_name = builder.name;
        meshComponent.m_vertex = std::move(builder.vertices);
        meshComponent.m_index = std::move(builder.indices);
        meshComponent.m_numVertex =
            static_cast<int>(meshComponent.m_vertex.size());
        meshComponent.m_numIndex =
            static_cast<int>(meshComponent.m_index.size());
        meshComponent.m_materialSlot = builder.materialSlot;

        m_meshes.push_back(std::move(meshComponent));
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
