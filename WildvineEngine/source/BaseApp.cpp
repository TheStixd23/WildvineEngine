#include "BaseApp.h"
#include "ResourceManager.h"
#include <fstream>
#include <iomanip>
#include <commdlg.h>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <cstring>

#pragma comment(lib, "Comdlg32.lib")

namespace {

	static std::string toLowerCopy(std::string s) { for (char& c : s) if (c >= 'A' && c <= 'Z') c = (char)(c + 32); return s; }
	static std::string stripExt(const std::string& f) { size_t d = f.find_last_of('.'); return (d == std::string::npos) ? f : f.substr(0, d); }
	static std::string fileBaseName(const std::string& path) { size_t s = path.find_last_of("/\\"); std::string n = (s == std::string::npos) ? path : path.substr(s + 1); return stripExt(n); }
	static bool endsWith(const std::string& s, const std::string& suf) { return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0; }
	static bool containsStr(const std::string& s, const std::string& sub) { return s.find(sub) != std::string::npos; }

	enum class SceneActorKind {
		Empty = 0,
		Model = 1,
		Light = 2
	};

	struct SceneActorRecord {
		int sceneIndex = -1;
		int parentIndex = -1;
		SceneActorKind kind = SceneActorKind::Empty;
		std::string name;
		std::string sourcePath;
		EU::Vector3 position;
		EU::Vector3 rotation;
		EU::Vector3 scale = EU::Vector3(1.0f, 1.0f, 1.0f);
		bool active = true;
		bool visible = true;
		bool castShadow = true;
		bool receiveShadow = true;
		bool selectable = true;
		bool hasLight = false;
		LightData lightData{};
		bool followLightPosition = true;
		bool followLightDirection = false;
		std::vector<MaterialParams> materialParams;
	};

	struct SceneDocument {
		EU::Vector3 cameraPosition = EU::Vector3(0.0f, 1.55f, -7.0f);
		EU::Vector3 cameraForward = EU::Vector3(0.0f, 0.0f, 1.0f);
		EU::Vector3 cameraUp = EU::Vector3(0.0f, 1.0f, 0.0f);
		int selectedActorIndex = -1;
		std::vector<SceneActorRecord> actors;
	};

	static bool expectSceneToken(
		std::istream& input,
		const char* expectedToken) {
		std::string token;
		if (!(input >> token)) return false;
		return token == expectedToken;
	}

	static std::string directoryOfPath(const std::string& path) {
		const size_t separator = path.find_last_of("/\\");
		return separator == std::string::npos
			? std::string()
			: path.substr(0, separator);
	}

	static std::string joinPath(
		const std::string& left,
		const std::string& right) {
		if (left.empty()) return right;
		if (right.empty()) return left;
		const char last = left[left.size() - 1];
		if (last == '\\' || last == '/') return left + right;
		return left + "\\" + right;
	}

	static bool isAbsolutePathString(const std::string& path) {
		if (path.size() >= 2 && path[1] == ':') return true;
		return path.size() >= 2 &&
			((path[0] == '\\' && path[1] == '\\') ||
			 (path[0] == '/' && path[1] == '/'));
	}

	static bool isFiniteVector(const EU::Vector3& value) {
		return std::isfinite(value.x) &&
			std::isfinite(value.y) &&
			std::isfinite(value.z);
	}

	static bool isFiniteMaterialParams(const MaterialParams& params) {
		return std::isfinite(params.baseColor.x) &&
			std::isfinite(params.baseColor.y) &&
			std::isfinite(params.baseColor.z) &&
			std::isfinite(params.baseColor.w) &&
			std::isfinite(params.metallic) &&
			std::isfinite(params.roughness) &&
			std::isfinite(params.ao) &&
			std::isfinite(params.normalScale) &&
			std::isfinite(params.emissiveStrength) &&
			std::isfinite(params.alphaCutoff);
	}

	static unsigned long long fnv1aAppend(
		unsigned long long hash,
		const void* data,
		size_t size) {
		const unsigned char* bytes =
			static_cast<const unsigned char*>(data);
		for (size_t index = 0; index < size; ++index) {
			hash ^= static_cast<unsigned long long>(bytes[index]);
			hash *= 1099511628211ull;
		}
		return hash;
	}

	template<typename T>
	static unsigned long long fnv1aValue(
		unsigned long long hash,
		const T& value) {
		return fnv1aAppend(hash, &value, sizeof(T));
	}

	static unsigned long long fnv1aString(
		unsigned long long hash,
		const std::string& value) {
		hash = fnv1aValue(hash, value.size());
		return value.empty()
			? hash
			: fnv1aAppend(hash, value.data(), value.size());
	}

	enum CarTextureId {
		CarTexWhite = 0,
		CarTexBlack,
		CarTexDarkGray,
		CarTexGray,
		CarTexFlatNormal,
		CarTexMetallicBlack,
		CarTexMetallicWhite,
		CarTexRoughnessGloss,
		CarTexRoughnessMedium,
		CarTexRoughnessMatte,
		CarTexRoughnessRubber,
		CarTexAOWide,
		CarTexGlass,
		CarTexMirror,
		CarTexDetail,
		CarTexInterior,
		CarTexLights,
		CarTexWheel,
		CarTexUndercarriage,
		CarTexGrille1,
		CarTexGrille2,
		CarTexTireSidewall,
		CarTexTireTread,
		CarTexStitchesNormal,
		CarTexEmissiveBlack
	};

	enum CarMaterialId {
		CarMatPaint = 0,
		CarMatPaintDark,
		CarMatChrome,
		CarMatBlack,
		CarMatMatte,
		CarMatRubber,
		CarMatGlass,
		CarMatLights,
		CarMatInterior,
		CarMatWheel,
		CarMatTire,
		CarMatTireTread,
		CarMatGrille1,
		CarMatGrille2,
		CarMatDetail,
		CarMatUndercarriage,
		CarMatBrake,
		CarMatMirror,
		CarMatEmissive
	};

	static unsigned int classifyCarMaterial(const std::string& rawName) {
		const std::string name = toLowerCopy(rawName);

		// Cuando Model3D separa una geometria por materiales, el nombre
		// queda como: Nodo__mat_NombreMaterial.
		// Se revisa primero el material real para que el nombre del nodo
		// "tireb" no convierta tambien el rin en goma.
		if (containsStr(name, "__mat_material #24"))
			return CarMatTire;
		if (containsStr(name, "__mat_material #25"))
			return CarMatTireTread;
		if (containsStr(name, "__mat_wheel_black"))
			return CarMatBlack;
		if (containsStr(name, "__mat_outer_rim"))
			return CarMatChrome;
		if (containsStr(name, "__mat_inner_rim") ||
			containsStr(name, "__mat_rim"))
			return CarMatWheel;

		if (containsStr(name, "gauge_emissive"))
			return CarMatEmissive;
		if (containsStr(name, "grille1"))
			return CarMatGrille1;
		if (containsStr(name, "grille2"))
			return CarMatGrille2;

		if (containsStr(name, "detail_glass") ||
			containsStr(name, "lights_glass") ||
			containsStr(name, "head_light") ||
			containsStr(name, "tail_light") ||
			containsStr(name, "reflector"))
			return CarMatLights;

		if (containsStr(name, "window") ||
			containsStr(name, "_glass_") ||
			containsStr(name, "__mat_glass"))
			return CarMatGlass;

		if (containsStr(name, "mirrorleft") ||
			containsStr(name, "mirrormiddle"))
			return CarMatMirror;

		if (containsStr(name, "body_2"))
			return CarMatPaintDark;
		if (containsStr(name, "_body_"))
			return CarMatPaint;

		if (containsStr(name, "tread"))
			return CarMatTireTread;
		if (containsStr(name, "tire") ||
			containsStr(name, "sidewall"))
			return CarMatTire;

		if (containsStr(name, "outer_rim"))
			return CarMatChrome;
		if (containsStr(name, "inner_rim") ||
			containsStr(name, "_rim") ||
			containsStr(name, "wheel"))
			return CarMatWheel;

		if (containsStr(name, "undercarriage"))
			return CarMatUndercarriage;
		if (containsStr(name, "caliper") ||
			containsStr(name, "_brake_"))
			return CarMatBrake;
		if (containsStr(name, "chrome") ||
			containsStr(name, "_metal_") ||
			containsStr(name, "_hub_"))
			return CarMatChrome;
		if (containsStr(name, "badge") ||
			containsStr(name, "emblem"))
			return CarMatDetail;

		if (containsStr(name, "interior") ||
			containsStr(name, "seat") ||
			containsStr(name, "steering") ||
			containsStr(name, "gauge") ||
			containsStr(name, "leather") ||
			containsStr(name, "plastic") ||
			containsStr(name, "mottled") ||
			containsStr(name, "stitch"))
			return CarMatInterior;

		if (containsStr(name, "rubber"))
			return CarMatRubber;
		if (containsStr(name, "black"))
			return CarMatBlack;

		if (containsStr(name, "matte") ||
			containsStr(name, "frame") ||
			containsStr(name, "bottom") ||
			containsStr(name, "misc") ||
			containsStr(name, "solid"))
			return CarMatMatte;

		return CarMatMatte;
	}

	static ExtensionType extFromName(const std::string& lower) {
        if (endsWith(lower, ".jpg") || endsWith(lower, ".jpeg")) return JPG;
        if (endsWith(lower, ".dds")) return DDS;
        if (endsWith(lower, ".tga")) return TGA;
        if (endsWith(lower, ".bmp")) return BMP;
        return PNG;
    }
	static std::vector<std::string> listImageFiles(const std::string& dir) {
		std::vector<std::string> out; std::string pat = dir + "\\*"; WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA(pat.c_str(), &fd); if (h == INVALID_HANDLE_VALUE) return out;
		do {
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
			std::string lo = toLowerCopy(fd.cFileName);
			if (endsWith(lo, ".png") || endsWith(lo, ".jpg") || endsWith(lo, ".jpeg") ||
                endsWith(lo, ".dds") || endsWith(lo, ".tga") || endsWith(lo, ".bmp"))
				out.push_back(fd.cFileName);
		} while (FindNextFileA(h, &fd));
		FindClose(h); return out;
	}

	static bool filePathExists(const std::string& path) {
		if (path.empty()) return false;
		const DWORD attributes = GetFileAttributesA(path.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES &&
			(attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	static std::string fileNameFromPath(const std::string& path) {
		const size_t separator = path.find_last_of("/\\");
		return separator == std::string::npos
			? path
			: path.substr(separator + 1);
	}

	static std::string normalizeAssetName(const std::string& value) {
		std::string result;
		result.reserve(value.size());
		for (char character : toLowerCopy(value)) {
			if ((character >= 'a' && character <= 'z') ||
				(character >= '0' && character <= '9')) {
				result.push_back(character);
			}
		}
		return result;
	}

	static bool isImagePath(const std::string& path) {
		const std::string lower = toLowerCopy(path);
		return endsWith(lower, ".png") ||
			endsWith(lower, ".jpg") ||
			endsWith(lower, ".jpeg") ||
			endsWith(lower, ".dds") ||
			endsWith(lower, ".tga") ||
			endsWith(lower, ".bmp");
	}

	static void collectImagePathsRecursive(
		const std::string& directory,
		int remainingDepth,
		std::vector<std::string>& output) {

		if (directory.empty() || remainingDepth < 0) {
			return;
		}

		WIN32_FIND_DATAA data{};
		const std::string pattern = joinPath(directory, "*");
		HANDLE search = FindFirstFileA(pattern.c_str(), &data);

		if (search == INVALID_HANDLE_VALUE) {
			return;
		}

		do {
			const std::string name = data.cFileName;
			if (name == "." || name == "..") {
				continue;
			}

			const std::string fullPath =
				joinPath(directory, name);

			if ((data.dwFileAttributes &
				FILE_ATTRIBUTE_DIRECTORY) != 0) {
				if (remainingDepth > 0) {
					collectImagePathsRecursive(
						fullPath,
						remainingDepth - 1,
						output);
				}
			}
			else if (isImagePath(name)) {
				output.push_back(fullPath);
			}
		} while (FindNextFileA(search, &data));

		FindClose(search);
	}

	enum class TextureSemantic {
		Albedo,
		Normal,
		Metallic,
		Roughness,
		AO,
		Emissive
	};

	static bool containsAnyToken(
		const std::string& text,
		const std::vector<std::string>& tokens) {
		for (const std::string& token : tokens) {
			if (text.find(token) != std::string::npos) {
				return true;
			}
		}
		return false;
	}

	static int textureSemanticScore(
		const std::string& candidatePath,
		TextureSemantic semantic,
		const std::string& materialName,
		const std::string& modelName) {

		const std::string normalizedFile =
			normalizeAssetName(
				stripExt(
					fileNameFromPath(
						candidatePath)));
		const std::string normalizedPath =
			normalizeAssetName(candidatePath);
		const std::string normalizedMaterial =
			normalizeAssetName(materialName);
		const std::string normalizedModel =
			normalizeAssetName(modelName);

		static const std::vector<std::string> tokenGroups[] = {
			{ "basecolor", "basecolour", "albedo", "diffuse", "color", "colour" },
			{ "normal", "normalmap", "nrm", "nor", "bump" },
			{ "metallic", "metalness", "metal" },
			{ "roughness", "rough", "rgh" },
			{ "ambientocclusion", "occlusion", "ao" },
			{ "emissive", "emission", "emit", "glow" }
		};

		const size_t semanticIndex =
			static_cast<size_t>(semantic);

		if (semanticIndex >=
			(sizeof(tokenGroups) /
			 sizeof(tokenGroups[0])) ||
			!containsAnyToken(
				normalizedFile,
				tokenGroups[semanticIndex])) {
			return -1000;
		}

		int score = 40;
		bool hasContextMatch = false;

		if (!normalizedMaterial.empty() &&
			normalizedMaterial != "default" &&
			normalizedPath.find(normalizedMaterial) !=
				std::string::npos) {
			score += 100;
			hasContextMatch = true;
		}

		if (!normalizedModel.empty()) {
			bool modelMatch =
				normalizedPath.find(normalizedModel) !=
					std::string::npos;

			// Algunos paquetes acortan la carpeta de texturas. Por ejemplo:
			// AlfaRomeo33Stradale.fbx -> Assets/Textures/AlfaRomeo33.
			if (!modelMatch && normalizedModel.size() >= 8) {
				for (size_t prefixLength = normalizedModel.size();
					prefixLength >= 8;
					--prefixLength) {
					if (normalizedPath.find(
						normalizedModel.substr(0, prefixLength)) !=
						std::string::npos) {
						modelMatch = true;
						break;
					}
				}
			}

			if (modelMatch) {
				score += 80;
				hasContextMatch = true;
			}
		}

		for (size_t groupIndex = 0;
			groupIndex <
				(sizeof(tokenGroups) /
				 sizeof(tokenGroups[0]));
			++groupIndex) {

			if (groupIndex == semanticIndex) {
				continue;
			}

			if (containsAnyToken(
				normalizedFile,
				tokenGroups[groupIndex])) {
				score -= 40;
			}
		}

		// No se toma una textura global de otro modelo solo porque contiene
		// "basecolor" o "normal". Debe coincidir con el modelo o material.
		if (!hasContextMatch) {
			score -= 60;
		}

		return score;
	}

	static std::string resolveReferencedTexturePath(
		const std::string& reference,
		const std::string& modelPath,
		const std::vector<std::string>& searchRoots,
		const std::vector<std::string>& indexedImages) {

		if (reference.empty()) {
			return std::string();
		}

		std::string normalizedReference = reference;
		for (char& character : normalizedReference) {
			if (character == '/') character = '\\';
		}

		std::vector<std::string> directCandidates;
		directCandidates.push_back(normalizedReference);

		const std::string modelDirectory =
			directoryOfPath(modelPath);
		const std::string referenceFileName =
			fileNameFromPath(normalizedReference);

		if (!modelDirectory.empty()) {
			directCandidates.push_back(
				joinPath(modelDirectory, normalizedReference));
			directCandidates.push_back(
				joinPath(modelDirectory, referenceFileName));
		}

		for (const std::string& root : searchRoots) {
			directCandidates.push_back(
				joinPath(root, normalizedReference));
			directCandidates.push_back(
				joinPath(root, referenceFileName));
		}

		for (const std::string& candidate : directCandidates) {
			if (filePathExists(candidate)) {
				return candidate;
			}
		}

		const std::string wantedName =
			normalizeAssetName(
				stripExt(referenceFileName));

		for (const std::string& candidate : indexedImages) {
			if (normalizeAssetName(
				stripExt(
					fileNameFromPath(candidate))) ==
				wantedName) {
				return candidate;
			}
		}

		return std::string();
	}

	static std::string findTextureBySemantic(
		TextureSemantic semantic,
		const std::string& materialName,
		const std::string& modelName,
		const std::vector<std::string>& indexedImages) {

		int bestScore = -1000;
		std::string bestPath;

		for (const std::string& candidate : indexedImages) {
			const int score = textureSemanticScore(
				candidate,
				semantic,
				materialName,
				modelName);

			if (score > bestScore) {
				bestScore = score;
				bestPath = candidate;
			}
		}

		return bestScore > 0
			? bestPath
			: std::string();
	}

	static bool rayTriangleIntersection(
		const XMFLOAT3& rayOrigin,
		const XMFLOAT3& rayDirection,
		const XMFLOAT3& v0,
		const XMFLOAT3& v1,
		const XMFLOAT3& v2,
		float& outDistance) {

		const float epsilon = 1e-7f;

		const float edge1x = v1.x - v0.x;
		const float edge1y = v1.y - v0.y;
		const float edge1z = v1.z - v0.z;

		const float edge2x = v2.x - v0.x;
		const float edge2y = v2.y - v0.y;
		const float edge2z = v2.z - v0.z;

		const float px = rayDirection.y * edge2z - rayDirection.z * edge2y;
		const float py = rayDirection.z * edge2x - rayDirection.x * edge2z;
		const float pz = rayDirection.x * edge2y - rayDirection.y * edge2x;

		const float determinant = edge1x * px + edge1y * py + edge1z * pz;
		if (fabsf(determinant) < epsilon) return false;

		const float inverseDeterminant = 1.0f / determinant;

		const float tx = rayOrigin.x - v0.x;
		const float ty = rayOrigin.y - v0.y;
		const float tz = rayOrigin.z - v0.z;

		const float u = (tx * px + ty * py + tz * pz) * inverseDeterminant;
		if (u < 0.0f || u > 1.0f) return false;

		const float qx = ty * edge1z - tz * edge1y;
		const float qy = tz * edge1x - tx * edge1z;
		const float qz = tx * edge1y - ty * edge1x;

		const float v =
			(rayDirection.x * qx +
			 rayDirection.y * qy +
			 rayDirection.z * qz) * inverseDeterminant;

		if (v < 0.0f || (u + v) > 1.0f) return false;

		const float distance =
			(edge2x * qx + edge2y * qy + edge2z * qz) *
			inverseDeterminant;

		if (distance <= epsilon) return false;

		outDistance = distance;
		return true;
	}

	static std::vector<std::string> listSubfolders(const std::string& dir) {
		std::vector<std::string> out; std::string pat = dir + "\\*"; WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA(pat.c_str(), &fd); if (h == INVALID_HANDLE_VALUE) return out;
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
			std::string n = fd.cFileName; if (n == "." || n == "..") continue;
			out.push_back(n);
		} while (FindNextFileA(h, &fd));
		FindClose(h); return out;
	}


	class TransformCommand : public ICommand {
	public:
		TransformCommand(EU::TSharedPointer<Actor> actor,
			const GizmoEditState& before,
			const GizmoEditState& after)
			: m_actor(actor), m_before(before), m_after(after) {
		}
		void undo() override { apply(m_before); }
		void redo() override { apply(m_after); }
		const char* name() const override { return "Transform"; }
	private:
		void apply(const GizmoEditState& s) {
			if (m_actor.isNull()) return;
			EU::TSharedPointer<Transform> t = m_actor->getComponent<Transform>();
			if (!t) return;
			t->setPosition(s.position);
			t->setRotation(s.rotation);
			t->setScale(s.scale);
		}
		EU::TSharedPointer<Actor> m_actor;
		GizmoEditState m_before;
		GizmoEditState m_after;
	};

	class SpawnActorCommand : public ICommand {
	public:
		SpawnActorCommand(
			BaseApp* app,
			EU::TSharedPointer<Actor> actor,
			EU::TSharedPointer<Actor> parent = EU::TSharedPointer<Actor>())
			: m_app(app), m_actor(actor), m_parent(parent) {
		}

		void undo() override {
			if (m_app) m_app->removeActorFromScene(m_actor);
		}

		void redo() override {
			if (!m_app) return;
			m_app->addActorToScene(m_actor);
			if (!m_parent.isNull()) {
				m_app->reparentActor(m_actor, m_parent, false);
			}
		}

		const char* name() const override { return "Spawn Actor"; }

	private:
		BaseApp* m_app;
		EU::TSharedPointer<Actor> m_actor;
		EU::TSharedPointer<Actor> m_parent;
	};

	class DeleteActorCommand : public ICommand {
	public:
		DeleteActorCommand(
			BaseApp* app,
			EU::TSharedPointer<Actor> actor,
			EU::TSharedPointer<Actor> parent,
			const std::vector<EU::TSharedPointer<Actor>>& children)
			: m_app(app),
			  m_actor(actor),
			  m_parent(parent),
			  m_children(children) {
		}

		void undo() override {
			if (!m_app) return;
			m_app->addActorToScene(m_actor);

			if (!m_parent.isNull()) {
				m_app->reparentActor(m_actor, m_parent, false);
			}

			for (const auto& child : m_children) {
				if (!child.isNull()) {
					m_app->reparentActor(child, m_actor, true);
				}
			}
		}

		void redo() override {
			if (m_app) m_app->removeActorFromScene(m_actor);
		}

		const char* name() const override { return "Delete Actor"; }

	private:
		BaseApp* m_app;
		EU::TSharedPointer<Actor> m_actor;
		EU::TSharedPointer<Actor> m_parent;
		std::vector<EU::TSharedPointer<Actor>> m_children;
	};

	class ReparentActorCommand : public ICommand {
	public:
		ReparentActorCommand(
			BaseApp* app,
			EU::TSharedPointer<Actor> child,
			EU::TSharedPointer<Actor> previousParent,
			EU::TSharedPointer<Actor> newParent)
			: m_app(app),
			  m_child(child),
			  m_previousParent(previousParent),
			  m_newParent(newParent) {
		}

		void undo() override {
			if (m_app) {
				m_app->reparentActor(
					m_child,
					m_previousParent,
					true);
			}
		}

		void redo() override {
			if (m_app) {
				m_app->reparentActor(m_child, m_newParent, true);
			}
		}

		const char* name() const override { return "Reparent Actor"; }

	private:
		BaseApp* m_app;
		EU::TSharedPointer<Actor> m_child;
		EU::TSharedPointer<Actor> m_previousParent;
		EU::TSharedPointer<Actor> m_newParent;
	};

} // namespace

HRESULT
BaseApp::awake() {
	HRESULT hr = S_OK;
	m_sceneGraph.init();
	MESSAGE("Main", "Awake", "Application awake successfully.");
	return hr;
}

int
BaseApp::run(HINSTANCE hInst, int nCmdShow) {
	if (FAILED(m_window.init(hInst, nCmdShow, WndProc, this))) {
		ERROR("Main", "Run", "Failed to initialize window.");
		return 0;
	}
	if (FAILED(awake())) {
		ERROR("Main", "Run", "Failed to awake application.");
		return 0;
	}
	if (FAILED(init())) {
		ERROR("Main", "Run", "Failed to initialize device and device context.");
		return 0;
	}
	m_gui.init(m_window, m_device, m_deviceContext);
	m_guiInitialized = true;

	MSG msg = {};
	LARGE_INTEGER freq, prev;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&prev);
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			LARGE_INTEGER curr;
			QueryPerformanceCounter(&curr);
			float deltaTime = static_cast<float>(curr.QuadPart - prev.QuadPart) / freq.QuadPart;
			prev = curr;
			update(deltaTime);
			render();
		}
	}
	return (int)msg.wParam;
}

HRESULT
BaseApp::init() {
	HRESULT hr = S_OK;

	hr = m_swapChain.init(m_device, m_deviceContext, m_backBuffer, m_window);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", ("Failed SwapChain. HRESULT: " + std::to_string(hr)).c_str()); return hr; }

	hr = m_renderTargetView.init(m_device, m_backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed RTV."); return hr; }

	hr = m_depthStencil.init(m_device, m_window.m_width, m_window.m_height,
		DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL, 4, 0);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed DepthStencil."); return hr; }

	hr = m_depthStencilView.init(m_device, m_depthStencil, DXGI_FORMAT_D24_UNORM_S8_UINT);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed DSV."); return hr; }

	hr = m_viewport.init(m_window);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed Viewport."); return hr; }
	m_d3dReady = true;

	std::array<std::string, 6> faces = {
		"Skybox/cubemap_0.png", "Skybox/cubemap_1.png", "Skybox/cubemap_2.png",
		"Skybox/cubemap_3.png", "Skybox/cubemap_4.png", "Skybox/cubemap_5.png"
	};
	m_skyboxTex.CreateCubemap(m_device, m_deviceContext, faces, false);

	// ---- Cargar Alfa Romeo 33 Stradale ----
	std::vector<MeshComponent> carMeshes;
	m_carModel = new Model3D(
		"Assets/Models/AlfaRomeo33Stradale.fbx",
		ModelType::FBX);

	carMeshes = m_carModel->GetMeshes();
	m_carCpuMeshes = carMeshes;

	if (carMeshes.empty()) {
		ERROR(
			"Main",
			"InitDevice",
			"AlfaRomeo33Stradale.fbx no contiene mallas o no pudo cargarse.");
		return E_FAIL;
	}

	const std::string textureRoot =
		"Assets/Textures/AlfaRomeo33/";

	const char* texturePaths[kCarTextureCount] = {
		"white",
		"black",
		"dark_gray",
		"gray",
		"flat_normal",
		"metallic_black",
		"metallic_white",
		"roughness_gloss",
		"roughness_medium",
		"roughness_matte",
		"roughness_rubber",
		"ao_white",
		"glass",
		"mirror",
		"detail_rgba",
		"interior_rgba",
		"lights_rgba",
		"wheel_rgba",
		"undercarriage_rgba",
		"grille1_rgba",
		"grille2_rgba",
		"tire_sidewall",
		"tire_tread",
		"stitches_normal",
		"emissive_black"
	};

	for (size_t textureIndex = 0;
		textureIndex < kCarTextureCount;
		++textureIndex) {

		hr = m_carTextures[textureIndex].init(
			m_device,
			textureRoot + texturePaths[textureIndex],
			PNG);

		if (FAILED(hr)) {
			ERROR(
				"Main",
				"InitDevice",
				("No se pudo cargar textura del auto: " +
					textureRoot +
					texturePaths[textureIndex] +
					".png").c_str());
			return hr;
		}
	}

	m_carRenderMesh.destroy();

	for (const MeshComponent& meshComponent : carMeshes) {
		Submesh submesh{};

		hr = submesh.vertexBuffer.init(
			m_device,
			meshComponent,
			D3D11_BIND_VERTEX_BUFFER);
		if (FAILED(hr)) {
			ERROR("Main", "InitDevice", "Fallo vertex buffer del auto.");
			return hr;
		}

		hr = submesh.indexBuffer.init(
			m_device,
			meshComponent,
			D3D11_BIND_INDEX_BUFFER);
		if (FAILED(hr)) {
			ERROR("Main", "InitDevice", "Fallo index buffer del auto.");
			return hr;
		}

		submesh.indexCount = meshComponent.m_numIndex;
		submesh.startIndex = 0;
		submesh.materialSlot =
			classifyCarMaterial(meshComponent.m_name);

		m_carRenderMesh.getSubmeshes().push_back(
			std::move(submesh));
	}

	m_carModelLocalMin =
		EU::Vector3(1e9f, 1e9f, 1e9f);
	m_carModelLocalMax =
		EU::Vector3(-1e9f, -1e9f, -1e9f);

	for (const MeshComponent& meshComponent : carMeshes) {
		for (const SimpleVertex& vertex : meshComponent.m_vertex) {
			m_carModelLocalMin.x =
				fminf(m_carModelLocalMin.x, vertex.Position.x);
			m_carModelLocalMin.y =
				fminf(m_carModelLocalMin.y, vertex.Position.y);
			m_carModelLocalMin.z =
				fminf(m_carModelLocalMin.z, vertex.Position.z);
			m_carModelLocalMax.x =
				fmaxf(m_carModelLocalMax.x, vertex.Position.x);
			m_carModelLocalMax.y =
				fmaxf(m_carModelLocalMax.y, vertex.Position.y);
			m_carModelLocalMax.z =
				fmaxf(m_carModelLocalMax.z, vertex.Position.z);
		}
	}

	LayoutBuilder builder;
	builder.Add("POSITION", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("BITANGENT", DXGI_FORMAT_R32G32B32_FLOAT)
		.Add("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT);

	hr = m_shaderProgram.init(
		m_device,
		"PBRShader.hlsl",
		builder);
	if (FAILED(hr)) {
		ERROR("Main", "InitDevice", "Failed ShaderProgram.");
		return hr;
	}

	hr = m_constantBuffer.init(
		m_device,
		sizeof(CBMain));
	if (FAILED(hr)) {
		ERROR("Main", "InitDevice", "Failed constant buffer.");
		return hr;
	}

	m_camera.setLens(
		XM_PIDIV4,
		m_window.m_width / (float)m_window.m_height,
		0.01f,
		250.0f);
	m_camera.setPosition(0.0f, 1.55f, -7.0f);

	m_constantBufferStruct.LightColor =
		EU::Vector3(1.0f, 0.98f, 0.95f);
	m_constantBufferStruct.LightDir =
		EU::Vector3(-0.35f, -1.0f, 0.45f);

	m_skybox.init(
		m_device,
		&m_deviceContext,
		m_skyboxTex);

	hr = m_defaultRasterizer.init(
		m_device,
		D3D11_FILL_SOLID,
		D3D11_CULL_NONE,
		false,
		true);
	if (FAILED(hr)) {
		ERROR("Main", "InitDevice", "Failed Rasterizer.");
		return hr;
	}

	hr = m_defaultDepthStencil.init(
		m_device,
		true,
		D3D11_DEPTH_WRITE_MASK_ALL,
		D3D11_COMPARISON_LESS);
	if (FAILED(hr)) {
		ERROR("Main", "InitDevice", "Failed DepthStencilState.");
		return hr;
	}

	hr = m_defaultSampler.init(m_device);
	if (FAILED(hr)) {
		ERROR("Main", "InitDevice", "Failed SamplerState.");
		return hr;
	}

	auto setupBaseMaterial =
		[this](Material& material,
			MaterialDomain domain,
			BlendMode blendMode) {
			material.setShader(&m_shaderProgram);
			material.setRasterizerState(&m_defaultRasterizer);
			material.setDepthStencilState(&m_defaultDepthStencil);
			material.setSamplerState(&m_defaultSampler);
			material.setDomain(domain);
			material.setBlendMode(blendMode);
		};

	setupBaseMaterial(
		m_pbrMaterial,
		MaterialDomain::Opaque,
		BlendMode::Opaque);
	setupBaseMaterial(
		m_maskedPbrMaterial,
		MaterialDomain::Masked,
		BlendMode::Opaque);
	setupBaseMaterial(
		m_transparentPbrMaterial,
		MaterialDomain::Transparent,
		BlendMode::Alpha);

	auto setupMaterial =
		[this](
			unsigned int materialIndex,
			Material* baseMaterial,
			unsigned int albedo,
			unsigned int normal,
			unsigned int metallic,
			unsigned int roughness,
			unsigned int ao,
			unsigned int emissive,
			const XMFLOAT4& baseColor,
			float metallicValue,
			float roughnessValue,
			float emissiveStrength,
			float alphaCutoff) {
			MaterialInstance& instance =
				m_carMaterials[materialIndex];

			instance.setMaterial(baseMaterial);
			instance.setAlbedo(&m_carTextures[albedo]);
			instance.setNormal(&m_carTextures[normal]);
			instance.setMetallic(&m_carTextures[metallic]);
			instance.setRoughness(&m_carTextures[roughness]);
			instance.setAO(&m_carTextures[ao]);
			instance.setEmissive(&m_carTextures[emissive]);

			instance.getParams().baseColor = baseColor;
			instance.getParams().metallic = metallicValue;
			instance.getParams().roughness = roughnessValue;
			instance.getParams().ao = 1.0f;
			instance.getParams().normalScale = 1.0f;
			instance.getParams().emissiveStrength =
				emissiveStrength;
			instance.getParams().alphaCutoff = alphaCutoff;
		};

	setupMaterial(CarMatPaint, &m_pbrMaterial,
		CarTexWhite, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessGloss, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.86f, 0.015f, 0.01f, 1.0f),
		0.05f, 0.16f, 0.0f, 0.5f);

	setupMaterial(CarMatPaintDark, &m_pbrMaterial,
		CarTexWhite, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessMedium, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.10f, 0.008f, 0.008f, 1.0f),
		0.03f, 0.28f, 0.0f, 0.5f);

	setupMaterial(CarMatChrome, &m_pbrMaterial,
		CarTexWhite, CarTexFlatNormal, CarTexMetallicWhite,
		CarTexRoughnessGloss, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.92f, 0.94f, 0.96f, 1.0f),
		1.0f, 0.08f, 0.0f, 0.5f);

	setupMaterial(CarMatBlack, &m_pbrMaterial,
		CarTexBlack, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessMedium, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		0.0f, 0.48f, 0.0f, 0.5f);

	setupMaterial(CarMatMatte, &m_pbrMaterial,
		CarTexDarkGray, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessMatte, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		0.0f, 0.72f, 0.0f, 0.5f);

	setupMaterial(CarMatRubber, &m_pbrMaterial,
		CarTexDarkGray, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessRubber, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.65f, 0.65f, 0.65f, 1.0f),
		0.0f, 0.92f, 0.0f, 0.5f);

	setupMaterial(CarMatGlass, &m_transparentPbrMaterial,
		CarTexGlass, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessGloss, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.56f, 0.70f, 0.80f, 0.30f),
		0.0f, 0.06f, 0.0f, 0.0f);

	setupMaterial(CarMatLights, &m_transparentPbrMaterial,
		CarTexLights, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessMedium, CarTexAOWide, CarTexLights,
		XMFLOAT4(1.0f, 1.0f, 1.0f, 0.88f),
		0.0f, 0.18f, 0.25f, 0.0f);

	setupMaterial(CarMatInterior, &m_pbrMaterial,
		CarTexInterior, CarTexStitchesNormal, CarTexMetallicBlack,
		CarTexRoughnessMatte, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.80f, 0.80f, 0.80f, 1.0f),
		0.0f, 0.62f, 0.0f, 0.5f);

	setupMaterial(CarMatWheel, &m_pbrMaterial,
		CarTexWheel, CarTexFlatNormal, CarTexMetallicWhite,
		CarTexRoughnessMedium, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		0.75f, 0.30f, 0.0f, 0.5f);

	setupMaterial(CarMatTire, &m_pbrMaterial,
		CarTexTireSidewall, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessRubber, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.50f, 0.50f, 0.50f, 1.0f),
		0.0f, 0.95f, 0.0f, 0.5f);

	setupMaterial(CarMatTireTread, &m_pbrMaterial,
		CarTexTireTread, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessRubber, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.42f, 0.42f, 0.42f, 1.0f),
		0.0f, 0.98f, 0.0f, 0.5f);

	setupMaterial(CarMatGrille1, &m_maskedPbrMaterial,
		CarTexGrille1, CarTexFlatNormal, CarTexMetallicWhite,
		CarTexRoughnessMedium, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.35f, 0.35f, 0.38f, 1.0f),
		0.70f, 0.38f, 0.0f, 0.40f);

	setupMaterial(CarMatGrille2, &m_maskedPbrMaterial,
		CarTexGrille2, CarTexFlatNormal, CarTexMetallicWhite,
		CarTexRoughnessMedium, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.28f, 0.28f, 0.30f, 1.0f),
		0.65f, 0.40f, 0.0f, 0.40f);

	setupMaterial(CarMatDetail, &m_pbrMaterial,
		CarTexDetail, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessMedium, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		0.10f, 0.40f, 0.0f, 0.5f);

	setupMaterial(CarMatUndercarriage, &m_pbrMaterial,
		CarTexUndercarriage, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessMatte, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.55f, 0.55f, 0.55f, 1.0f),
		0.10f, 0.78f, 0.0f, 0.5f);

	setupMaterial(CarMatBrake, &m_pbrMaterial,
		CarTexGray, CarTexFlatNormal, CarTexMetallicWhite,
		CarTexRoughnessMedium, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.70f, 0.72f, 0.75f, 1.0f),
		0.85f, 0.42f, 0.0f, 0.5f);

	setupMaterial(CarMatMirror, &m_pbrMaterial,
		CarTexMirror, CarTexFlatNormal, CarTexMetallicWhite,
		CarTexRoughnessGloss, CarTexAOWide, CarTexEmissiveBlack,
		XMFLOAT4(0.95f, 0.98f, 1.0f, 1.0f),
		1.0f, 0.03f, 0.0f, 0.5f);

	setupMaterial(CarMatEmissive, &m_pbrMaterial,
		CarTexInterior, CarTexFlatNormal, CarTexMetallicBlack,
		CarTexRoughnessMedium, CarTexAOWide, CarTexInterior,
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		0.0f, 0.45f, 0.85f, 0.5f);

	m_car01 = EU::MakeShared<Actor>(m_device);
	if (m_car01.isNull()) {
		ERROR("Main", "InitDevice", "No se pudo crear actor del auto.");
		return E_FAIL;
	}

	m_car01->setName("Alfa Romeo 33 Stradale");
	// El modelo de demostracion conserva la orientacion almacenada en el FBX.
	// No se aplica una rotacion correctiva especifica para este automovil.
	m_car01->getComponent<Transform>()->setTransform(
		EU::Vector3(0.0f, 0.10f, 5.6f),
		EU::Vector3(0.0f, 0.0f, 0.0f),
		EU::Vector3(1.0f, 1.0f, 1.0f));

	{
		EU::TSharedPointer<MeshRendererComponent> meshRenderer =
			m_car01->getComponent<MeshRendererComponent>();

		if (!meshRenderer) {
			meshRenderer =
				EU::MakeShared<MeshRendererComponent>();
			m_car01->addComponent(meshRenderer);
		}

		std::vector<MaterialInstance*> materialPointers;
		materialPointers.reserve(kCarMaterialCount);
		for (size_t materialIndex = 0;
			materialIndex < kCarMaterialCount;
			++materialIndex) {
			materialPointers.push_back(
				&m_carMaterials[materialIndex]);
		}

		meshRenderer->setMesh(&m_carRenderMesh);
		meshRenderer->setMaterialInstances(materialPointers);
		meshRenderer->setVisible(true);
		meshRenderer->setCastShadow(true);
	}

	m_actors.push_back(m_car01);
	m_sceneGraph.addEntity(m_car01.get());
	m_lastSelectedRenderableActor = m_car01;
	m_actorSourcePaths[m_car01.get()] =
		"Assets/Models/AlfaRomeo33Stradale.fbx";

	// Vista inicial de tres cuartos, parecida a la referencia.
	{
		const EU::Vector3 cameraEye(
			4.60f, 2.15f, -1.80f);
		const EU::Vector3 cameraTarget(
			0.0f, 0.55f, 5.60f);

		m_camera.lookAt(
			cameraEye,
			cameraTarget);
		m_camera.setPosition(
			cameraEye);
	}

	m_car02 = EU::TSharedPointer<Actor>();

	// ---- Luz direccional ----
	m_directionalLightActor = EU::MakeShared<Actor>(m_device);
	if (!m_directionalLightActor.isNull()) {
		m_directionalLightActor->setName("Directional Light");
		EU::TSharedPointer<LightComponent> lightComponent = m_directionalLightActor->getComponent<LightComponent>();
		if (!lightComponent) { lightComponent = EU::MakeShared<LightComponent>(); m_directionalLightActor->addComponent(lightComponent); }
		lightComponent->setType(LightType::Directional);
		lightComponent->setDirection(m_constantBufferStruct.LightDir);
		lightComponent->setColor(m_constantBufferStruct.LightColor);
		lightComponent->setIntensity(1.0f);
		lightComponent->setRange(12.0f);
		lightComponent->setCastShadow(true);
		lightComponent->setFollowTransformPosition(false);
		lightComponent->setFollowTransformDirection(false);
		m_actors.push_back(m_directionalLightActor);
		m_sceneGraph.addEntity(m_directionalLightActor.get());
	}

	hr = m_editorViewportPass.init(m_device, 1280, 720);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed EditorViewportPass."); return hr; }

	hr = m_renderPipeline.init(m_device, RendererType::Deferred);
	if (FAILED(hr)) { ERROR("Main", "InitDevice", "Failed RenderPipeline."); return hr; }

	buildTextureThumbnails();

	ensureSceneDirectories();
	m_savedSceneSignature = computeSceneSignature();
	m_sceneDirty = false;
	m_recoveryAvailable = fileExists(getAutosaveScenePath());
	m_gui.m_sceneDisplayName = getSceneDisplayName();
	m_gui.m_sceneDirty = false;
	m_gui.m_recoveryAvailable = m_recoveryAvailable;

	return S_OK;
}

std::string
BaseApp::getExecutableDirectory() const {
	char buffer[32768] = {};
	const DWORD length = GetModuleFileNameA(
		nullptr,
		buffer,
		static_cast<DWORD>(sizeof(buffer)));
	if (length == 0 || length >= sizeof(buffer)) {
		return ".";
	}
	return directoryOfPath(std::string(buffer, length));
}

bool
BaseApp::fileExists(const std::string& path) const {
	if (path.empty()) return false;
	const DWORD attributes = GetFileAttributesA(path.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES &&
		(attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool
BaseApp::ensureSceneDirectories() const {
	const std::string savedDirectory =
		joinPath(getExecutableDirectory(), "Saved");
	const std::string sceneDirectory =
		joinPath(savedDirectory, "Scenes");
	const std::string autosaveDirectory =
		joinPath(savedDirectory, "Autosaves");

	auto createDirectoryIfNeeded = [](const std::string& directory) {
		if (CreateDirectoryA(directory.c_str(), nullptr) != 0) {
			return true;
		}
		return GetLastError() == ERROR_ALREADY_EXISTS;
	};

	return createDirectoryIfNeeded(savedDirectory) &&
		createDirectoryIfNeeded(sceneDirectory) &&
		createDirectoryIfNeeded(autosaveDirectory);
}

std::string
BaseApp::getDefaultScenePath() const {
	return joinPath(
		joinPath(
			joinPath(getExecutableDirectory(), "Saved"),
			"Scenes"),
		"Untitled.wvscene");
}

std::string
BaseApp::getAutosaveScenePath() const {
	return joinPath(
		joinPath(
			joinPath(getExecutableDirectory(), "Saved"),
			"Autosaves"),
		"Recovery.wvscene");
}

std::string
BaseApp::getSceneDisplayName() const {
	return m_currentScenePath.empty()
		? "Sin titulo"
		: fileBaseName(m_currentScenePath);
}

bool
BaseApp::showOpenSceneDialog(std::string& outPath) const {
	outPath.clear();
	char fileName[32768] = {};
	const std::string initialDirectory = joinPath(
		joinPath(getExecutableDirectory(), "Saved"),
		"Scenes");

	OPENFILENAMEA dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner = m_window.m_hWnd;
	dialog.lpstrFilter =
		"Wildvine Scene (*.wvscene)\0*.wvscene\0"
		"Todos los archivos (*.*)\0*.*\0\0";
	dialog.lpstrFile = fileName;
	dialog.nMaxFile = static_cast<DWORD>(sizeof(fileName));
	dialog.lpstrInitialDir = initialDirectory.c_str();
	dialog.lpstrDefExt = "wvscene";
	dialog.Flags = OFN_FILEMUSTEXIST |
		OFN_PATHMUSTEXIST |
		OFN_HIDEREADONLY |
		OFN_NOCHANGEDIR;

	if (GetOpenFileNameA(&dialog) == FALSE) {
		return false;
	}
	outPath = fileName;
	return !outPath.empty();
}

bool
BaseApp::showSaveSceneDialog(std::string& outPath) const {
	outPath.clear();
	char fileName[32768] = {};
	strcpy_s(fileName, sizeof(fileName), "Untitled.wvscene");
	const std::string initialDirectory = joinPath(
		joinPath(getExecutableDirectory(), "Saved"),
		"Scenes");

	OPENFILENAMEA dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner = m_window.m_hWnd;
	dialog.lpstrFilter =
		"Wildvine Scene (*.wvscene)\0*.wvscene\0"
		"Todos los archivos (*.*)\0*.*\0\0";
	dialog.lpstrFile = fileName;
	dialog.nMaxFile = static_cast<DWORD>(sizeof(fileName));
	dialog.lpstrInitialDir = initialDirectory.c_str();
	dialog.lpstrDefExt = "wvscene";
	dialog.Flags = OFN_OVERWRITEPROMPT |
		OFN_PATHMUSTEXIST |
		OFN_NOCHANGEDIR;

	if (GetSaveFileNameA(&dialog) == FALSE) {
		return false;
	}
	outPath = fileName;
	if (!endsWith(toLowerCopy(outPath), ".wvscene")) {
		outPath += ".wvscene";
	}
	return true;
}

void
BaseApp::deleteAutosaveFile() {
	const std::string autosavePath = getAutosaveScenePath();
	if (fileExists(autosavePath)) {
		DeleteFileA(autosavePath.c_str());
	}
	m_recoveryAvailable = fileExists(autosavePath);
}

void
BaseApp::clearCurrentScene() {
	m_sceneGraph.destroy();
	m_sceneGraph.init();
	m_actors.clear();
	m_actorSourcePaths.clear();
	m_directionalLightActor.reset();
	m_lastSelectedRenderableActor.reset();
	m_gui.selectedActorIndex = -1;
	m_commands.clear();
	m_clipboard = ActorClipboard();
	m_hasClipboard = false;
	m_lightNameCounter = 1;
}

void
BaseApp::createDefaultSceneLight() {
	EU::TSharedPointer<Actor> lightActor = spawnLightActor(
		LightType::Directional,
		"Directional Light",
		EU::Vector3(0.0f, 3.0f, 0.0f),
		EU::Vector3(1.0f, 0.96f, 0.90f),
		1.0f,
		12.0f,
		true);
	if (lightActor.isNull()) return;

	EU::TSharedPointer<LightComponent> light =
		lightActor->getComponent<LightComponent>();
	if (light) {
		light->setDirection(EU::Vector3(-0.35f, -0.85f, 0.25f));
		light->setFollowTransformPosition(false);
		light->setFollowTransformDirection(false);
	}

	addActorToScene(lightActor);
	m_directionalLightActor = lightActor;
}

bool
BaseApp::newScene() {
	clearCurrentScene();
	createDefaultSceneLight();

	const EU::Vector3 cameraPosition(0.0f, 1.55f, -7.0f);
	m_camera.lookAt(
		cameraPosition,
		EU::Vector3(0.0f, 1.0f, 0.0f));
	m_camera.setPosition(cameraPosition);
	m_camera.updateViewMatrix();

	m_currentScenePath.clear();
	m_savedSceneSignature = computeSceneSignature();
	m_sceneDirty = false;
	m_autosaveTimer = 0.0f;
	deleteAutosaveFile();
	m_gui.m_statusMessage = "Nueva escena creada";
	m_gui.m_sceneDisplayName = getSceneDisplayName();
	m_gui.m_sceneDirty = false;
	MESSAGE("BaseApp", "newScene", "Nueva escena creada");
	return true;
}

bool
BaseApp::saveSceneInternal(
	const std::string& path,
	bool updateEditorState) {
	if (path.empty()) return false;
	if (!ensureSceneDirectories()) {
		ERROR("BaseApp", "saveScene", "No se pudieron crear las carpetas de escenas");
		return false;
	}

	const std::string temporaryPath = path + ".tmp";
	std::ofstream output(temporaryPath.c_str(), std::ios::out | std::ios::trunc);
	if (!output.is_open()) {
		ERROR("BaseApp", "saveScene", "No se pudo abrir el archivo temporal");
		return false;
	}

	output << std::setprecision(9);
	output << "WILDVINE_SCENE 1\n";
	const EU::Vector3 cameraPosition = m_camera.getPosition();
	const EU::Vector3 cameraForward = m_camera.GetForward();
	const EU::Vector3 cameraUp = m_camera.GetUp();
	output << "CAMERA "
		<< cameraPosition.x << ' ' << cameraPosition.y << ' ' << cameraPosition.z << ' '
		<< cameraForward.x << ' ' << cameraForward.y << ' ' << cameraForward.z << ' '
		<< cameraUp.x << ' ' << cameraUp.y << ' ' << cameraUp.z << '\n';
	output << "SELECTED " << m_gui.selectedActorIndex << '\n';
	output << "ACTOR_COUNT " << m_actors.size() << '\n';

	std::unordered_map<const Actor*, int> actorIndices;
	for (int index = 0; index < static_cast<int>(m_actors.size()); ++index) {
		if (!m_actors[index].isNull()) {
			actorIndices[m_actors[index].get()] = index;
		}
	}

	for (int index = 0; index < static_cast<int>(m_actors.size()); ++index) {
		const EU::TSharedPointer<Actor> actor = m_actors[index];
		if (actor.isNull()) continue;

		EU::TSharedPointer<Transform> transform =
			actor->getComponent<Transform>();
		EU::TSharedPointer<MeshRendererComponent> renderer =
			actor->getComponent<MeshRendererComponent>();
		EU::TSharedPointer<LightComponent> light =
			actor->getComponent<LightComponent>();

		SceneActorKind kind = SceneActorKind::Empty;
		if (light) kind = SceneActorKind::Light;
		else if (renderer && renderer->hasMesh()) kind = SceneActorKind::Model;

		int parentIndex = -1;
		Actor* parent = dynamic_cast<Actor*>(m_sceneGraph.getParent(actor.get()));
		const auto parentFound = actorIndices.find(parent);
		if (parentFound != actorIndices.end()) {
			parentIndex = parentFound->second;
		}

		std::string sourcePath;
		const auto sourceFound = m_actorSourcePaths.find(actor.get());
		if (sourceFound != m_actorSourcePaths.end()) {
			sourcePath = sourceFound->second;
		}
		if (kind == SceneActorKind::Empty && !sourcePath.empty()) {
			kind = SceneActorKind::Model;
		}

		const EU::Vector3 position = transform
			? transform->getPosition()
			: EU::Vector3();
		const EU::Vector3 rotation = transform
			? transform->getRotation()
			: EU::Vector3();
		const EU::Vector3 scale = transform
			? transform->getScale()
			: EU::Vector3(1.0f, 1.0f, 1.0f);

		output << "ACTOR\n";
		output << "INDEX " << index << '\n';
		output << "PARENT " << parentIndex << '\n';
		output << "KIND " << static_cast<int>(kind) << '\n';
		output << "NAME " << std::quoted(actor->getName()) << '\n';
		output << "SOURCE " << std::quoted(sourcePath) << '\n';
		output << "ACTIVE " << (actor->isActive() ? 1 : 0) << '\n';
		output << "TRANSFORM "
			<< position.x << ' ' << position.y << ' ' << position.z << ' '
			<< rotation.x << ' ' << rotation.y << ' ' << rotation.z << ' '
			<< scale.x << ' ' << scale.y << ' ' << scale.z << '\n';
		output << "RENDERER "
			<< (renderer && renderer->isVisible() ? 1 : 0) << ' '
			<< (renderer && renderer->canCastShadow() ? 1 : 0) << ' '
			<< (renderer && renderer->canReceiveShadow() ? 1 : 0) << ' '
			<< (renderer && renderer->isSelectable() ? 1 : 0) << '\n';

		if (light) {
			const LightData& data = light->getLightData();
			output << "LIGHT 1 "
				<< static_cast<int>(data.type) << ' '
				<< (data.enabled ? 1 : 0) << ' '
				<< (data.castShadow ? 1 : 0) << ' '
				<< (light->followsTransformPosition() ? 1 : 0) << ' '
				<< (light->followsTransformDirection() ? 1 : 0) << '\n';
			output << "LIGHT_COLOR "
				<< data.color.x << ' ' << data.color.y << ' ' << data.color.z << '\n';
			output << "LIGHT_POSITION "
				<< data.position.x << ' ' << data.position.y << ' ' << data.position.z << '\n';
			output << "LIGHT_DIRECTION "
				<< data.direction.x << ' ' << data.direction.y << ' ' << data.direction.z << '\n';
			output << "LIGHT_PARAMS "
				<< data.intensity << ' ' << data.range << ' '
				<< data.innerSpotAngle << ' ' << data.spotAngle << '\n';
		}
		else {
			output << "LIGHT 0 0 0 0 0 0\n";
			output << "LIGHT_COLOR 1 1 1\n";
			output << "LIGHT_POSITION 0 0 0\n";
			output << "LIGHT_DIRECTION 0 -1 0\n";
			output << "LIGHT_PARAMS 1 10 20 35\n";
		}

		const std::vector<MaterialInstance*> materials = renderer
			? renderer->getMaterialInstances()
			: std::vector<MaterialInstance*>();
		output << "MATERIAL_COUNT " << materials.size() << '\n';
		for (MaterialInstance* material : materials) {
			const MaterialParams params = material
				? material->getParams()
				: MaterialParams();
			output << "MATERIAL "
				<< params.baseColor.x << ' '
				<< params.baseColor.y << ' '
				<< params.baseColor.z << ' '
				<< params.baseColor.w << ' '
				<< params.metallic << ' '
				<< params.roughness << ' '
				<< params.ao << ' '
				<< params.normalScale << ' '
				<< params.emissiveStrength << ' '
				<< params.alphaCutoff << '\n';
		}
		output << "END_ACTOR\n";
	}
	output << "END_SCENE\n";
	output.flush();
	const bool writeSucceeded = output.good();
	output.close();

	if (!writeSucceeded) {
		DeleteFileA(temporaryPath.c_str());
		ERROR("BaseApp", "saveScene", "La escritura de la escena quedo incompleta");
		return false;
	}

	if (fileExists(path)) {
		const std::string backupPath = path + ".bak";
		CopyFileA(path.c_str(), backupPath.c_str(), FALSE);
	}

	if (MoveFileExA(
		temporaryPath.c_str(),
		path.c_str(),
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE) {
		DeleteFileA(temporaryPath.c_str());
		ERROR("BaseApp", "saveScene", "No se pudo reemplazar el archivo de escena");
		return false;
	}

	if (updateEditorState) {
		m_currentScenePath = path;
		m_savedSceneSignature = computeSceneSignature();
		m_sceneDirty = false;
		m_autosaveTimer = 0.0f;
		deleteAutosaveFile();
		m_gui.m_statusMessage = "Escena guardada: " + getSceneDisplayName();
		m_gui.m_sceneDisplayName = getSceneDisplayName();
		m_gui.m_sceneDirty = false;
		MESSAGE("BaseApp", "saveScene", "Escena guardada correctamente");
	}
	return true;
}

bool
BaseApp::saveScene(const std::string& path) {
	return saveSceneInternal(path, true);
}

bool
BaseApp::loadSceneInternal(
	const std::string& path,
	bool recoveredAutosave) {
	std::ifstream input(path.c_str(), std::ios::in);
	if (!input.is_open()) {
		ERROR("BaseApp", "loadScene", "No se pudo abrir la escena");
		return false;
	}

	std::string magic;
	int version = 0;
	if (!(input >> magic >> version) ||
		magic != "WILDVINE_SCENE" ||
		version != 1) {
		ERROR("BaseApp", "loadScene", "Formato o version de escena no compatible");
		return false;
	}

	SceneDocument document;
	if (!expectSceneToken(input, "CAMERA") ||
		!(input >> document.cameraPosition.x
			>> document.cameraPosition.y
			>> document.cameraPosition.z
			>> document.cameraForward.x
			>> document.cameraForward.y
			>> document.cameraForward.z
			>> document.cameraUp.x
			>> document.cameraUp.y
			>> document.cameraUp.z) ||
		!expectSceneToken(input, "SELECTED") ||
		!(input >> document.selectedActorIndex) ||
		!expectSceneToken(input, "ACTOR_COUNT")) {
		ERROR("BaseApp", "loadScene", "Cabecera de escena incompleta");
		return false;
	}

	if (!isFiniteVector(document.cameraPosition) ||
		!isFiniteVector(document.cameraForward) ||
		!isFiniteVector(document.cameraUp)) {
		ERROR("BaseApp", "loadScene", "La camara contiene valores invalidos");
		return false;
	}

	size_t actorCount = 0;
	if (!(input >> actorCount) || actorCount > 10000) {
		ERROR("BaseApp", "loadScene", "Cantidad de actores invalida");
		return false;
	}
	document.actors.reserve(actorCount);

	for (size_t actorNumber = 0; actorNumber < actorCount; ++actorNumber) {
		SceneActorRecord record;
		int kind = 0;
		int active = 1;
		int visible = 1;
		int castShadow = 1;
		int receiveShadow = 1;
		int selectable = 1;
		int hasLight = 0;
		int lightType = 0;
		int lightEnabled = 1;
		int lightCastShadow = 0;
		int followPosition = 1;
		int followDirection = 0;

		if (!expectSceneToken(input, "ACTOR") ||
			!expectSceneToken(input, "INDEX") || !(input >> record.sceneIndex) ||
			!expectSceneToken(input, "PARENT") || !(input >> record.parentIndex) ||
			!expectSceneToken(input, "KIND") || !(input >> kind) ||
			!expectSceneToken(input, "NAME") || !(input >> std::quoted(record.name)) ||
			!expectSceneToken(input, "SOURCE") || !(input >> std::quoted(record.sourcePath)) ||
			!expectSceneToken(input, "ACTIVE") || !(input >> active) ||
			!expectSceneToken(input, "TRANSFORM") ||
			!(input >> record.position.x >> record.position.y >> record.position.z
				>> record.rotation.x >> record.rotation.y >> record.rotation.z
				>> record.scale.x >> record.scale.y >> record.scale.z) ||
			!expectSceneToken(input, "RENDERER") ||
			!(input >> visible >> castShadow >> receiveShadow >> selectable) ||
			!expectSceneToken(input, "LIGHT") ||
			!(input >> hasLight >> lightType >> lightEnabled >> lightCastShadow
				>> followPosition >> followDirection) ||
			!expectSceneToken(input, "LIGHT_COLOR") ||
			!(input >> record.lightData.color.x
				>> record.lightData.color.y
				>> record.lightData.color.z) ||
			!expectSceneToken(input, "LIGHT_POSITION") ||
			!(input >> record.lightData.position.x
				>> record.lightData.position.y
				>> record.lightData.position.z) ||
			!expectSceneToken(input, "LIGHT_DIRECTION") ||
			!(input >> record.lightData.direction.x
				>> record.lightData.direction.y
				>> record.lightData.direction.z) ||
			!expectSceneToken(input, "LIGHT_PARAMS") ||
			!(input >> record.lightData.intensity
				>> record.lightData.range
				>> record.lightData.innerSpotAngle
				>> record.lightData.spotAngle) ||
			!expectSceneToken(input, "MATERIAL_COUNT")) {
			ERROR("BaseApp", "loadScene", "Registro de actor incompleto");
			return false;
		}

		size_t materialCount = 0;
		if (!(input >> materialCount) || materialCount > 1024) {
			ERROR("BaseApp", "loadScene", "Cantidad de materiales invalida");
			return false;
		}
		record.materialParams.reserve(materialCount);
		for (size_t materialIndex = 0;
			materialIndex < materialCount;
			++materialIndex) {
			MaterialParams params;
			if (!expectSceneToken(input, "MATERIAL") ||
				!(input >> params.baseColor.x
					>> params.baseColor.y
					>> params.baseColor.z
					>> params.baseColor.w
					>> params.metallic
					>> params.roughness
					>> params.ao
					>> params.normalScale
					>> params.emissiveStrength
					>> params.alphaCutoff)) {
				ERROR("BaseApp", "loadScene", "Material de escena incompleto");
				return false;
			}
			if (!isFiniteMaterialParams(params)) {
				ERROR("BaseApp", "loadScene", "El material contiene valores invalidos");
				return false;
			}
			record.materialParams.push_back(params);
		}

		if (!expectSceneToken(input, "END_ACTOR")) {
			ERROR("BaseApp", "loadScene", "Falta END_ACTOR");
			return false;
		}

		if (record.sceneIndex != static_cast<int>(actorNumber) ||
			kind < static_cast<int>(SceneActorKind::Empty) ||
			kind > static_cast<int>(SceneActorKind::Light)) {
			ERROR("BaseApp", "loadScene", "Indice o tipo de actor invalido");
			return false;
		}

		if (!isFiniteVector(record.position) ||
			!isFiniteVector(record.rotation) ||
			!isFiniteVector(record.scale) ||
			!isFiniteVector(record.lightData.color) ||
			!isFiniteVector(record.lightData.position) ||
			!isFiniteVector(record.lightData.direction) ||
			!std::isfinite(record.lightData.intensity) ||
			!std::isfinite(record.lightData.range) ||
			!std::isfinite(record.lightData.innerSpotAngle) ||
			!std::isfinite(record.lightData.spotAngle)) {
			ERROR("BaseApp", "loadScene", "El actor contiene valores numericos invalidos");
			return false;
		}

		record.kind = static_cast<SceneActorKind>(kind);
		record.active = active != 0;
		record.visible = visible != 0;
		record.castShadow = castShadow != 0;
		record.receiveShadow = receiveShadow != 0;
		record.selectable = selectable != 0;
		record.hasLight = hasLight != 0 ||
			record.kind == SceneActorKind::Light;
		record.lightData.type = static_cast<LightType>(
			lightType < 0 || lightType > 2 ? 0 : lightType);
		record.lightData.enabled = lightEnabled != 0;
		record.lightData.castShadow = lightCastShadow != 0;
		record.followLightPosition = followPosition != 0;
		record.followLightDirection = followDirection != 0;
		document.actors.push_back(record);
	}

	if (!expectSceneToken(input, "END_SCENE")) {
		ERROR("BaseApp", "loadScene", "Falta END_SCENE");
		return false;
	}

	for (const SceneActorRecord& record : document.actors) {
		if (record.parentIndex < -1 ||
			record.parentIndex >= static_cast<int>(document.actors.size()) ||
			record.parentIndex == record.sceneIndex) {
			ERROR("BaseApp", "loadScene", "Jerarquia de escena invalida");
			return false;
		}

		int currentParent = record.parentIndex;
		for (size_t depth = 0;
			currentParent >= 0 && depth <= document.actors.size();
			++depth) {
			if (currentParent == record.sceneIndex ||
				depth == document.actors.size()) {
				ERROR("BaseApp", "loadScene", "Se detecto un ciclo en la jerarquia guardada");
				return false;
			}
			currentParent = document.actors[currentParent].parentIndex;
		}
	}

	clearCurrentScene();
	std::vector<EU::TSharedPointer<Actor>> createdActors;
	createdActors.reserve(document.actors.size());
	bool missingResources = false;
	const std::string sceneDirectory = directoryOfPath(path);

	for (const SceneActorRecord& record : document.actors) {
		EU::TSharedPointer<Actor> actor;
		if (record.kind == SceneActorKind::Light || record.hasLight) {
			actor = spawnLightActor(
				record.lightData.type,
				record.name,
				record.position,
				record.lightData.color,
				record.lightData.intensity,
				record.lightData.range,
				record.lightData.castShadow);
		}
		else if (record.kind == SceneActorKind::Model) {
			std::string sourcePath = record.sourcePath;
			const std::string lowerSource = toLowerCopy(sourcePath);
			const bool builtInCar =
				lowerSource == toLowerCopy("Assets/Models/AlfaRomeo33Stradale.fbx");

			if (!sourcePath.empty() && !builtInCar && !fileExists(sourcePath) &&
				!isAbsolutePathString(sourcePath)) {
				const std::string fromScene = joinPath(sceneDirectory, sourcePath);
				const std::string fromExecutable = joinPath(
					getExecutableDirectory(),
					sourcePath);
				if (fileExists(fromScene)) sourcePath = fromScene;
				else if (fileExists(fromExecutable)) sourcePath = fromExecutable;
			}

			if (!sourcePath.empty()) {
				actor = spawnActorFromSource(
					sourcePath,
					record.name,
					record.position,
					record.rotation,
					record.scale);
			}

			if (actor.isNull()) {
				missingResources = true;
				actor = EU::MakeShared<Actor>(m_device);
				if (!actor.isNull()) {
					actor->setName(record.name + " [Recurso faltante]");
					m_actorSourcePaths[actor.get()] = record.sourcePath;
				}
			}
		}
		else {
			actor = EU::MakeShared<Actor>(m_device);
			if (!actor.isNull()) actor->setName(record.name);
		}

		if (actor.isNull()) {
			ERROR("BaseApp", "loadScene", "No se pudo crear un actor de la escena");
			clearCurrentScene();
			createDefaultSceneLight();
			return false;
		}

		actor->setName(record.name);
		actor->setActive(record.active);
		EU::TSharedPointer<Transform> transform =
			actor->getComponent<Transform>();
		if (transform) {
			transform->setTransform(
				record.position,
				record.rotation,
				record.scale);
		}

		EU::TSharedPointer<MeshRendererComponent> renderer =
			actor->getComponent<MeshRendererComponent>();
		if (renderer) {
			renderer->setVisible(record.visible);
			renderer->setCastShadow(record.castShadow);
			renderer->setReceiveShadow(record.receiveShadow);
			renderer->setSelectable(record.selectable);
			actor->setCastShadow(record.castShadow);

			const std::vector<MaterialInstance*>& materials =
				renderer->getMaterialInstances();
			const size_t restoreCount = (std::min)(
				materials.size(),
				record.materialParams.size());
			for (size_t materialIndex = 0;
				materialIndex < restoreCount;
				++materialIndex) {
				if (materials[materialIndex]) {
					materials[materialIndex]->getParams() =
						record.materialParams[materialIndex];
				}
			}
		}

		EU::TSharedPointer<LightComponent> light =
			actor->getComponent<LightComponent>();
		if (light && record.hasLight) {
			light->setType(record.lightData.type);
			light->setEnabled(record.lightData.enabled);
			light->setColor(record.lightData.color);
			light->setIntensity(record.lightData.intensity);
			light->setRange(record.lightData.range);
			light->setPosition(record.lightData.position);
			light->setDirection(record.lightData.direction);
			light->setSpotAngles(
				record.lightData.innerSpotAngle,
				record.lightData.spotAngle);
			light->setCastShadow(record.lightData.castShadow);
			light->setFollowTransformPosition(record.followLightPosition);
			light->setFollowTransformDirection(record.followLightDirection);
		}

		addActorToScene(actor);
		createdActors.push_back(actor);
	}

	for (const SceneActorRecord& record : document.actors) {
		if (record.parentIndex >= 0) {
			reparentActor(
				createdActors[record.sceneIndex],
				createdActors[record.parentIndex],
				false);
		}
	}
	m_sceneGraph.validateHierarchy(true);

	m_directionalLightActor.reset();
	for (const auto& actor : m_actors) {
		if (actor.isNull()) continue;
		EU::TSharedPointer<LightComponent> light =
			actor->getComponent<LightComponent>();
		if (light && light->getType() == LightType::Directional) {
			m_directionalLightActor = actor;
			break;
		}
	}

	EU::Vector3 forward = document.cameraForward.normalize();
	EU::Vector3 up = document.cameraUp.normalize();
	if (forward.isNearlyZero()) forward = EU::Vector3(0.0f, 0.0f, 1.0f);
	if (up.isNearlyZero()) up = EU::Vector3(0.0f, 1.0f, 0.0f);
	m_camera.lookAt(
		document.cameraPosition,
		document.cameraPosition + forward,
		up);
	m_camera.setPosition(document.cameraPosition);
	m_camera.updateViewMatrix();

	m_gui.selectedActorIndex =
		document.selectedActorIndex >= 0 &&
		document.selectedActorIndex < static_cast<int>(m_actors.size())
		? document.selectedActorIndex
		: -1;
	m_commands.clear();
	m_autosaveTimer = 0.0f;

	if (recoveredAutosave) {
		m_currentScenePath.clear();
		m_savedSceneSignature = 0ull;
		m_sceneDirty = true;
		m_gui.m_statusMessage = missingResources
			? "Autoguardado recuperado con recursos faltantes"
			: "Autoguardado recuperado. Usa Guardar como...";
	}
	else {
		m_currentScenePath = path;
		m_savedSceneSignature = computeSceneSignature();
		m_sceneDirty = false;
		deleteAutosaveFile();
		m_gui.m_statusMessage = missingResources
			? "Escena abierta con recursos faltantes"
			: "Escena abierta: " + getSceneDisplayName();
	}

	m_gui.m_sceneDisplayName = getSceneDisplayName();
	m_gui.m_sceneDirty = m_sceneDirty;
	MESSAGE("BaseApp", "loadScene", "Escena cargada correctamente");
	return true;
}

bool
BaseApp::loadScene(const std::string& path) {
	return loadSceneInternal(path, false);
}

unsigned long long
BaseApp::computeSceneSignature() const {
	unsigned long long hash = 1469598103934665603ull;
	hash = fnv1aValue(hash, m_actors.size());

	std::unordered_map<const Actor*, int> actorIndices;
	for (int index = 0; index < static_cast<int>(m_actors.size()); ++index) {
		if (!m_actors[index].isNull()) actorIndices[m_actors[index].get()] = index;
	}

	for (const auto& actor : m_actors) {
		const bool valid = !actor.isNull();
		hash = fnv1aValue(hash, valid);
		if (!valid) continue;
		hash = fnv1aString(hash, actor->getName());
		hash = fnv1aValue(hash, actor->isActive());

		const auto sourceFound = m_actorSourcePaths.find(actor.get());
		hash = fnv1aString(hash, sourceFound == m_actorSourcePaths.end()
			? std::string()
			: sourceFound->second);

		EU::TSharedPointer<Transform> transform = actor->getComponent<Transform>();
		if (transform) {
			const EU::Vector3 position = transform->getPosition();
			const EU::Vector3 rotation = transform->getRotation();
			const EU::Vector3 scale = transform->getScale();
			hash = fnv1aValue(hash, position.x);
			hash = fnv1aValue(hash, position.y);
			hash = fnv1aValue(hash, position.z);
			hash = fnv1aValue(hash, rotation.x);
			hash = fnv1aValue(hash, rotation.y);
			hash = fnv1aValue(hash, rotation.z);
			hash = fnv1aValue(hash, scale.x);
			hash = fnv1aValue(hash, scale.y);
			hash = fnv1aValue(hash, scale.z);
		}

		Actor* parent = dynamic_cast<Actor*>(m_sceneGraph.getParent(actor.get()));
		const auto parentFound = actorIndices.find(parent);
		const int parentIndex = parentFound == actorIndices.end()
			? -1
			: parentFound->second;
		hash = fnv1aValue(hash, parentIndex);

		EU::TSharedPointer<MeshRendererComponent> renderer =
			actor->getComponent<MeshRendererComponent>();
		const bool hasRenderer = renderer && renderer->hasMesh();
		hash = fnv1aValue(hash, hasRenderer);
		if (renderer) {
			hash = fnv1aValue(hash, renderer->isVisible());
			hash = fnv1aValue(hash, renderer->canCastShadow());
			hash = fnv1aValue(hash, renderer->canReceiveShadow());
			hash = fnv1aValue(hash, renderer->isSelectable());
			for (MaterialInstance* material : renderer->getMaterialInstances()) {
				if (!material) continue;
				const MaterialParams& params = material->getParams();
				hash = fnv1aValue(hash, params.baseColor.x);
				hash = fnv1aValue(hash, params.baseColor.y);
				hash = fnv1aValue(hash, params.baseColor.z);
				hash = fnv1aValue(hash, params.baseColor.w);
				hash = fnv1aValue(hash, params.metallic);
				hash = fnv1aValue(hash, params.roughness);
				hash = fnv1aValue(hash, params.ao);
				hash = fnv1aValue(hash, params.normalScale);
				hash = fnv1aValue(hash, params.emissiveStrength);
				hash = fnv1aValue(hash, params.alphaCutoff);
			}
		}

		EU::TSharedPointer<LightComponent> light =
			actor->getComponent<LightComponent>();
		const bool hasLight = !light.isNull();
		hash = fnv1aValue(hash, hasLight);
		if (light) {
			const LightData& data = light->getLightData();
			const int type = static_cast<int>(data.type);
			hash = fnv1aValue(hash, type);
			hash = fnv1aAppend(hash, &data.color, sizeof(data.color));
			hash = fnv1aValue(hash, data.intensity);
			hash = fnv1aAppend(hash, &data.direction, sizeof(data.direction));
			hash = fnv1aValue(hash, data.range);
			hash = fnv1aAppend(hash, &data.position, sizeof(data.position));
			hash = fnv1aValue(hash, data.innerSpotAngle);
			hash = fnv1aValue(hash, data.spotAngle);
			hash = fnv1aValue(hash, data.enabled);
			hash = fnv1aValue(hash, data.castShadow);
			hash = fnv1aValue(hash, light->followsTransformPosition());
			hash = fnv1aValue(hash, light->followsTransformDirection());
		}
	}
	return hash;
}

void
BaseApp::updateSceneDirtyState() {
	const unsigned long long currentSignature = computeSceneSignature();
	m_sceneDirty = currentSignature != m_savedSceneSignature;
	m_gui.m_sceneDirty = m_sceneDirty;
	m_gui.m_sceneDisplayName = getSceneDisplayName();
	m_recoveryAvailable = fileExists(getAutosaveScenePath());
	m_gui.m_recoveryAvailable = m_recoveryAvailable;
}

void
BaseApp::update(float deltaTime) {
	handleEditorViewportResize();

	if (!m_initialStateCaptured) {
		captureInitialState();
		m_initialStateCaptured = true;
	}

	// Estado de escena visible en la barra superior antes de construir la GUI.
	updateSceneDirtyState();

	// GUI
	m_gui.update(m_viewport, m_window);

	// Acciones de archivo. Los dialogos usan OFN_NOCHANGEDIR para no alterar
	// las rutas relativas de modelos, texturas o shaders del proyecto.
	{
		if (m_gui.consumeNewSceneRequest()) {
			newScene();
		}

		if (m_gui.consumeOpenSceneRequest()) {
			std::string scenePath;
			if (showOpenSceneDialog(scenePath)) {
				if (!loadScene(scenePath)) {
					m_gui.m_statusMessage = "No se pudo abrir la escena";
				}
			}
		}

		if (m_gui.consumeSaveSceneAsRequest()) {
			std::string scenePath;
			if (showSaveSceneDialog(scenePath)) {
				if (!saveScene(scenePath)) {
					m_gui.m_statusMessage = "No se pudo guardar la escena";
				}
			}
		}

		if (m_gui.consumeSaveSceneRequest()) {
			if (m_currentScenePath.empty()) {
				std::string scenePath;
				if (showSaveSceneDialog(scenePath) &&
					!saveScene(scenePath)) {
					m_gui.m_statusMessage = "No se pudo guardar la escena";
				}
			}
			else if (!saveScene(m_currentScenePath)) {
				m_gui.m_statusMessage = "No se pudo guardar la escena";
			}
		}

		if (m_gui.consumeRecoverSceneRequest()) {
			const std::string autosavePath = getAutosaveScenePath();
			if (!fileExists(autosavePath) ||
				!loadSceneInternal(autosavePath, true)) {
				m_gui.m_statusMessage = "No se pudo recuperar el autoguardado";
			}
		}
	}

	m_gui.drawViewportPanel(m_editorViewportPass.getSRV());
	m_gui.drawViewportGrid(m_camera);
	m_gui.drawLightGizmos(m_actors, m_camera);

	if (!m_actors.empty() && m_gui.selectedActorIndex >= 0 &&
		m_gui.selectedActorIndex < (int)m_actors.size()) {
		m_gui.inspectorGeneral(m_actors[m_gui.selectedActorIndex]);
		m_gui.editTransform(m_camera, m_window, m_actors[m_gui.selectedActorIndex]);
	}
	m_gui.outliner(m_actors);

	// Reparentado solicitado desde el Outliner mediante drag & drop.
	{
		int childIndex = -1;
		int parentIndex = -1;
		if (m_gui.consumeReparentRequest(childIndex, parentIndex)) {
			const bool childValid =
				childIndex >= 0 &&
				childIndex < static_cast<int>(m_actors.size()) &&
				!m_actors[childIndex].isNull();
			const bool parentValid =
				parentIndex == -1 ||
				(parentIndex >= 0 &&
				 parentIndex < static_cast<int>(m_actors.size()) &&
				 !m_actors[parentIndex].isNull());

			if (childValid && parentValid) {
				EU::TSharedPointer<Actor> child = m_actors[childIndex];
				EU::TSharedPointer<Actor> newParent = parentIndex >= 0
					? m_actors[parentIndex]
					: EU::TSharedPointer<Actor>();

				Actor* previousParentRaw = dynamic_cast<Actor*>(
					m_sceneGraph.getParent(child.get()));
				EU::TSharedPointer<Actor> previousParent =
					findActorShared(previousParentRaw);

				if (previousParent.get() != newParent.get() &&
					reparentActor(child, newParent, true)) {
					m_commands.push(std::unique_ptr<ICommand>(
						new ReparentActorCommand(
							this,
							child,
							previousParent,
							newParent)));
					MESSAGE("BaseApp", "reparentActor",
						"Jerarquia actualizada desde el Outliner");
				}
			}
		}
	}

	{
		EU::TSharedPointer<Actor> selectedActor = getSelectedActor();
		if (selectedActor) {
			auto selectedRenderer =
				selectedActor->getComponent<MeshRendererComponent>();
			if (selectedRenderer && selectedRenderer->hasMesh()) {
				m_lastSelectedRenderableActor = selectedActor;
			}
		}
	}

	m_gui.drawGBufferDebugPanel(
		m_renderPipeline.getGBufferAlbedoMetallicSRV(),
		m_renderPipeline.getGBufferNormalRoughnessSRV(),
		m_renderPipeline.getGBufferWorldAoSRV(),
		m_renderPipeline.getGBufferEmissiveAlphaSRV());
	m_gui.drawRenderDebugPanel(
		m_renderPipeline.getPreShadowSRV(),
		m_editorViewportPass.getSRV(),
		m_renderPipeline.getShadowMapSRV());
	if (!m_directionalLightActor.isNull()) {
		auto mainLight = m_directionalLightActor->getComponent<LightComponent>();
		if (mainLight) {
			LightData& lightData = mainLight->getLightData();
			m_gui.drawLightingPanel(&lightData.direction.x, &lightData.color.x);
		}
	}
	m_gui.drawStatsPanel(deltaTime, m_lastDrawCalls);
	m_gui.drawTexturePreview();
	m_gui.drawConsolePanel();
	m_gui.drawContentBrowser(m_thumbnails);

	// Creacion de luces desde el menu Crear o desde la barra superior.
	{
		const EU::Vector3 cameraPosition = m_camera.getPosition();
		const EU::Vector3 cameraForward = m_camera.GetForward().normalize();
		const EU::Vector3 spawnPosition =
			cameraPosition + cameraForward * 3.0f;

		auto addRequestedLight = [&](LightType type, const char* baseName) {
			const std::string name = std::string(baseName) + " " +
				std::to_string(m_lightNameCounter++);
			EU::TSharedPointer<Actor> lightActor = spawnLightActor(
				type,
				name,
				spawnPosition,
				EU::Vector3(1.0f, 1.0f, 1.0f),
				type == LightType::Directional ? 1.0f : 3.0f,
				12.0f,
				false);
			if (lightActor) {
				addActorToScene(lightActor);
				m_gui.selectedActorIndex =
					static_cast<int>(m_actors.size()) - 1;
				if (type != LightType::Point &&
					m_lastSelectedRenderableActor) {
					aimLightAtActor(
						lightActor,
						m_lastSelectedRenderableActor);
				}
			}
		};

		if (m_gui.consumeCreateDirectionalLightRequest()) {
			addRequestedLight(LightType::Directional, "Directional Light");
		}
		if (m_gui.consumeCreatePointLightRequest()) {
			addRequestedLight(LightType::Point, "Point Light");
		}
		if (m_gui.consumeCreateSpotLightRequest()) {
			addRequestedLight(LightType::Spot, "Spot Light");
		}
		if (m_gui.consumeCreateStudioRigRequest()) {
			createStudioLightRig();
		}

		if (m_gui.consumeAimLightRequest()) {
			EU::TSharedPointer<Actor> selectedLight = getSelectedActor();
			if (selectedLight &&
				selectedLight->getComponent<LightComponent>() &&
				m_lastSelectedRenderableActor) {
				aimLightAtActor(
					selectedLight,
					m_lastSelectedRenderableActor);
			}
		}
	}

	// Instanciar el modelo exactamente en el punto donde se solto dentro
	// del viewport. Si no existe una superficie valida, se usa el plano Y=0.
	if (m_gui.m_assetSpawnRequested) {
		m_gui.m_assetSpawnRequested = false;
		EU::TSharedPointer<Actor> actor = loadModelActor(m_gui.m_assetSpawnPath);
		if (!actor.isNull()) {
			EU::Vector3 localMinimum;
			EU::Vector3 localMaximum;
			EU::Vector3 placementPosition;

			if (getActorAABB(actor, localMinimum, localMaximum) &&
				getViewportPlacementPosition(
					m_gui.m_assetSpawnScreenPosition,
					localMinimum,
					placementPosition)) {

				auto transform = actor->getComponent<Transform>();
				if (transform) transform->setPosition(placementPosition);
			}

			addActorToScene(actor);
			m_commands.push(std::unique_ptr<ICommand>(
				new SpawnActorCommand(this, actor)));
			m_gui.selectedActorIndex = static_cast<int>(m_actors.size()) - 1;
			m_gui.m_statusMessage = "Modelo importado: " + actor->getName();
			MESSAGE("BaseApp", "loadModelActor",
				"Modelo colocado directamente en el viewport");
		}
		else {
			m_gui.m_statusMessage = "No se pudo importar el modelo";
		}
	}


	if (m_gui.consumeResetRequest()) {
		resetSceneToDefaults();
	}

	// --- Undo/Redo: registrar movimientos del gizmo ---
	{
		bool usingGizmo = m_gui.m_isUsingGizmo;
		int sel = m_gui.selectedActorIndex;

		if (usingGizmo && !m_prevGizmoUsing) {
			m_gizmoEditActorIndex = sel;
			m_gizmoEditing = captureGizmoState(sel, m_gizmoBefore);
		}
		else if (!usingGizmo && m_prevGizmoUsing && m_gizmoEditing) {
			GizmoEditState after;
			if (captureGizmoState(m_gizmoEditActorIndex, after)) {
				const float eps = 1e-4f;
				bool changed =
					fabsf(after.position.x - m_gizmoBefore.position.x) > eps ||
					fabsf(after.position.y - m_gizmoBefore.position.y) > eps ||
					fabsf(after.position.z - m_gizmoBefore.position.z) > eps ||
					fabsf(after.rotation.x - m_gizmoBefore.rotation.x) > eps ||
					fabsf(after.rotation.y - m_gizmoBefore.rotation.y) > eps ||
					fabsf(after.rotation.z - m_gizmoBefore.rotation.z) > eps ||
					fabsf(after.scale.x - m_gizmoBefore.scale.x) > eps ||
					fabsf(after.scale.y - m_gizmoBefore.scale.y) > eps ||
					fabsf(after.scale.z - m_gizmoBefore.scale.z) > eps;
				if (changed && m_gizmoEditActorIndex >= 0 &&
					m_gizmoEditActorIndex < (int)m_actors.size()) {
					EU::TSharedPointer<Actor> actor = m_actors[m_gizmoEditActorIndex];
					m_commands.push(std::unique_ptr<ICommand>(
						new TransformCommand(actor, m_gizmoBefore, after)));
				}
			}
			m_gizmoEditing = false;
		}
		m_prevGizmoUsing = usingGizmo;
	}

	// --- Atajos Undo/Redo ---
	{
		ImGuiIO& io = ImGui::GetIO();
		bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
		static bool zPrev = false, yPrev = false;
		bool zNow = (GetAsyncKeyState('Z') & 0x8000) != 0;
		bool yNow = (GetAsyncKeyState('Y') & 0x8000) != 0;

		bool menuUndo = m_gui.consumeUndoRequest();
		bool menuRedo = m_gui.consumeRedoRequest();
		bool doUndo = menuUndo || (ctrl && zNow && !zPrev && !io.WantTextInput);
		bool doRedo = menuRedo || (ctrl && yNow && !yPrev && !io.WantTextInput);

		if (doUndo) { m_commands.undo(); MESSAGE("BaseApp", "undo", "Deshacer"); }
		if (doRedo) { m_commands.redo(); MESSAGE("BaseApp", "redo", "Rehacer"); }

		zPrev = zNow; yPrev = yNow;
	}

	// --- Atajos Duplicar / Copiar / Pegar / Borrar / Prefab ---
	{
		ImGuiIO& io = ImGui::GetIO();
		bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
		bool typing = io.WantTextInput;
		static bool dPrev = false, cPrev = false, vPrev = false, delPrev = false;
		bool dNow = (GetAsyncKeyState('D') & 0x8000) != 0;
		bool cNow = (GetAsyncKeyState('C') & 0x8000) != 0;
		bool vNow = (GetAsyncKeyState('V') & 0x8000) != 0;
		bool delNow = (GetAsyncKeyState(VK_DELETE) & 0x8000) != 0;

		bool doDup = (!typing && ctrl && dNow && !dPrev) || m_gui.m_duplicateRequested;
		bool doCopy = (!typing && ctrl && cNow && !cPrev) || m_gui.m_copyRequested;
		bool doPaste = (!typing && ctrl && vNow && !vPrev) || m_gui.m_pasteRequested;
		bool doDel = (!typing && delNow && !delPrev) || m_gui.m_deleteRequested;
		bool doSave = m_gui.m_savePrefabRequested;
		bool doLoad = m_gui.m_loadPrefabRequested;

		m_gui.m_duplicateRequested = false;
		m_gui.m_copyRequested = false;
		m_gui.m_pasteRequested = false;
		m_gui.m_deleteRequested = false;
		m_gui.m_savePrefabRequested = false;
		m_gui.m_loadPrefabRequested = false;

		if (doCopy) { MESSAGE("BaseApp", "input", "Copy detectado");      copySelected(); }
		if (doDup) { MESSAGE("BaseApp", "input", "Duplicate detectado"); duplicateSelected(); }
		if (doPaste) { MESSAGE("BaseApp", "input", "Paste detectado");     pasteClipboard(); }
		if (doDel) { MESSAGE("BaseApp", "input", "Delete detectado");    deleteSelected(); }
		if (doSave)  savePrefabSelected();
		if (doLoad)  loadPrefab();

		dPrev = dNow; cPrev = cNow; vPrev = vNow; delPrev = delNow;
	}

	// --- Navegacion de camara estilo Unreal / DCC ---
	// Clic derecho + mouse: mirar.
	// WASD + Q/E: desplazamiento libre.
	// Alt + clic izquierdo: orbitar alrededor del objeto seleccionado.
	// Clic central: pan.
	// Rueda: zoom; clic derecho + rueda: velocidad.
	if (!m_gui.m_isUsingGizmo) {
		ImGuiIO& io = ImGui::GetIO();

		if (m_gui.m_viewportHovered && !io.WantTextInput) {
			const bool ctrlPressed =
				(GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			const bool altPressed =
				(GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			const bool rightMouseDown =
				ImGui::IsMouseDown(ImGuiMouseButton_Right);
			const bool middleMouseDown =
				ImGui::IsMouseDown(ImGuiMouseButton_Middle);
			const bool leftMouseDown =
				ImGui::IsMouseDown(ImGuiMouseButton_Left);

			static float cameraMovementSpeed = 4.0f;

			if (rightMouseDown && io.MouseWheel != 0.0f) {
				cameraMovementSpeed += io.MouseWheel * 0.75f;
				if (cameraMovementSpeed < 1.0f) cameraMovementSpeed = 1.0f;
				if (cameraMovementSpeed > 30.0f) cameraMovementSpeed = 30.0f;
			}
			else if (!rightMouseDown && io.MouseWheel != 0.0f) {
				m_camera.walk(io.MouseWheel * 0.70f);
			}

			// Orbita DCC: Alt + clic izquierdo.
			if (altPressed && leftMouseDown) {
				if (!m_orbitActive) {
					m_orbitActive = true;

					bool pivotFound = false;
					if (m_gui.selectedActorIndex >= 0 &&
						m_gui.selectedActorIndex < (int)m_actors.size()) {

						EU::TSharedPointer<Actor> selected =
							m_actors[m_gui.selectedActorIndex];

						EU::Vector3 localMin, localMax;
						EU::TSharedPointer<Transform> transform =
							selected.isNull()
							? EU::TSharedPointer<Transform>()
							: selected->getComponent<Transform>();

						if (transform && getActorAABB(selected, localMin, localMax)) {
							XMVECTOR localCenter = XMVectorSet(
								(localMin.x + localMax.x) * 0.5f,
								(localMin.y + localMax.y) * 0.5f,
								(localMin.z + localMax.z) * 0.5f,
								1.0f);

							XMVECTOR worldCenter =
								XMVector3TransformCoord(localCenter, transform->worldMatrix);

							XMFLOAT3 center;
							XMStoreFloat3(&center, worldCenter);
							m_orbitPivot = EU::Vector3(center.x, center.y, center.z);
							pivotFound = true;
						}
					}

					if (!pivotFound) {
						EU::Vector3 position = m_camera.getPosition();
						EU::Vector3 forward = m_camera.GetForward();
						m_orbitPivot = EU::Vector3(
							position.x + forward.x * 5.0f,
							position.y + forward.y * 5.0f,
							position.z + forward.z * 5.0f);
					}

					EU::Vector3 position = m_camera.getPosition();
					const float dx = position.x - m_orbitPivot.x;
					const float dy = position.y - m_orbitPivot.y;
					const float dz = position.z - m_orbitPivot.z;
					m_orbitDistance = sqrtf(dx * dx + dy * dy + dz * dz);
					if (m_orbitDistance < 0.25f) m_orbitDistance = 0.25f;
				}

				ImGui::SetMouseCursor(ImGuiMouseCursor_None);

				EU::Vector3 position = m_camera.getPosition();
				XMVECTOR offset = XMVectorSet(
					position.x - m_orbitPivot.x,
					position.y - m_orbitPivot.y,
					position.z - m_orbitPivot.z,
					0.0f);

				const float orbitSensitivity = 0.0040f;

				XMMATRIX yawRotation =
					XMMatrixRotationY(-io.MouseDelta.x * orbitSensitivity);
				offset = XMVector3TransformNormal(offset, yawRotation);

				EU::Vector3 cameraRight = m_camera.GetRight();
				XMVECTOR rightAxis = XMVector3Normalize(XMVectorSet(
					cameraRight.x, cameraRight.y, cameraRight.z, 0.0f));

				XMMATRIX pitchRotation = XMMatrixRotationAxis(
					rightAxis,
					-io.MouseDelta.y * orbitSensitivity);
				offset = XMVector3TransformNormal(offset, pitchRotation);

				offset = XMVector3Normalize(offset) * m_orbitDistance;

				XMVECTOR pivot = XMVectorSet(
					m_orbitPivot.x,
					m_orbitPivot.y,
					m_orbitPivot.z,
					1.0f);

				XMVECTOR newPositionVector = pivot + offset;
				XMFLOAT3 newPosition;
				XMStoreFloat3(&newPosition, newPositionVector);

				EU::Vector3 eye(newPosition.x, newPosition.y, newPosition.z);
				m_camera.lookAt(eye, m_orbitPivot);
				m_camera.setPosition(eye);
			}
			else {
				m_orbitActive = false;
			}

			// Cámara libre tipo Unreal.
			if (rightMouseDown && !altPressed) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_None);
				const float mouseSensitivity = 0.0040f;
				m_camera.yaw(io.MouseDelta.x * mouseSensitivity);
				m_camera.pitch(io.MouseDelta.y * mouseSensitivity);
			}

			// Pan con clic central.
			if (middleMouseDown) {
				const float panSensitivity = 0.020f;
				const float horizontal = -io.MouseDelta.x * panSensitivity;
				const float vertical = io.MouseDelta.y * panSensitivity;

				m_camera.strafe(horizontal);

				EU::Vector3 cameraPosition = m_camera.getPosition();
				cameraPosition.y += vertical;
				m_camera.setPosition(cameraPosition);

				// Mantener el pivote coherente después de desplazar la vista.
				EU::Vector3 right = m_camera.GetRight();
				m_orbitPivot.x += right.x * horizontal;
				m_orbitPivot.y += right.y * horizontal + vertical;
				m_orbitPivot.z += right.z * horizontal;
			}

			if (!ctrlPressed && !altPressed) {
				float movementSpeed = cameraMovementSpeed;
				if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
					movementSpeed *= 2.5f;

				const float movement = movementSpeed * deltaTime;

				if ((GetAsyncKeyState('W') & 0x8000) != 0)
					m_camera.walk(movement);
				if ((GetAsyncKeyState('S') & 0x8000) != 0)
					m_camera.walk(-movement);
				if ((GetAsyncKeyState('A') & 0x8000) != 0)
					m_camera.strafe(-movement);
				if ((GetAsyncKeyState('D') & 0x8000) != 0)
					m_camera.strafe(movement);

				EU::Vector3 cameraPosition = m_camera.getPosition();
				bool verticalChanged = false;

				if ((GetAsyncKeyState('Q') & 0x8000) != 0) {
					cameraPosition.y -= movement;
					verticalChanged = true;
				}
				if ((GetAsyncKeyState('E') & 0x8000) != 0) {
					cameraPosition.y += movement;
					verticalChanged = true;
				}
				if (verticalChanged)
					m_camera.setPosition(cameraPosition);

				const float rotationSpeed = 1.5f * deltaTime;
				if ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0)
					m_camera.yaw(-rotationSpeed);
				if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0)
					m_camera.yaw(rotationSpeed);
				if ((GetAsyncKeyState(VK_UP) & 0x8000) != 0)
					m_camera.pitch(-rotationSpeed);
				if ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0)
					m_camera.pitch(rotationSpeed);
			}
		}
		else {
			m_orbitActive = false;
		}
	}

	// --- Focus (F) y Zoom-to-fit ---
	{
		EU::TSharedPointer<Actor> selected;
		if (!m_actors.empty() && m_gui.selectedActorIndex >= 0 &&
			m_gui.selectedActorIndex < (int)m_actors.size()) {
			selected = m_actors[m_gui.selectedActorIndex];
		}
		static bool fDown = false;
		bool fNow = (GetAsyncKeyState('F') & 0x8000) != 0;
		if (m_gui.m_viewportHovered && fNow && !fDown) focusCameraOnActor(selected);
		fDown = fNow;
		if (m_gui.consumeFocusRequest()) focusCameraOnActor(selected);
		if (m_gui.consumeFitRequest())   fitCameraToScene();
	}

	// --- Picking (click izquierdo) ---
	if (m_gui.m_viewportHovered && !m_gui.m_isUsingGizmo &&
		(GetAsyncKeyState(VK_MENU) & 0x8000) == 0 &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver()) {
		pickActorFromMouse();
	}

	// --- Outline del objeto seleccionado ---
	if (!m_actors.empty() && m_gui.selectedActorIndex >= 0 &&
		m_gui.selectedActorIndex < (int)m_actors.size()) {
		EU::TSharedPointer<Actor> sel = m_actors[m_gui.selectedActorIndex];
		if (!sel.isNull()) {
			EU::TSharedPointer<Transform> t = sel->getComponent<Transform>();
			EU::Vector3 mn, mx;
			if (t && getActorAABB(sel, mn, mx)) {
				m_gui.drawSelectionOutline(m_camera, mn, mx, t->worldMatrix);
			}
		}
	}


	// Resize estable del viewport
	unsigned int desiredW = static_cast<unsigned int>(m_gui.m_viewportSize.x);
	unsigned int desiredH = static_cast<unsigned int>(m_gui.m_viewportSize.y);
	const unsigned int kMinViewportSize = 64;
	if (desiredW < kMinViewportSize) desiredW = kMinViewportSize;
	if (desiredH < kMinViewportSize) desiredH = kMinViewportSize;
	if (desiredW != m_lastRequestedViewportWidth || desiredH != m_lastRequestedViewportHeight) {
		m_lastRequestedViewportWidth = desiredW;
		m_lastRequestedViewportHeight = desiredH;
		m_viewportResizeStableFrames = 0;
	}
	else {
		m_viewportResizeStableFrames++;
	}
	const int kStableFramesRequired = 2;
	if (m_viewportResizeStableFrames >= kStableFramesRequired) {
		if (desiredW != m_editorViewportPass.getWidth() ||
			desiredH != m_editorViewportPass.getHeight()) {
			m_editorViewportResizePending = true;
			m_pendingViewportWidth = desiredW;
			m_pendingViewportHeight = desiredH;
		}
	}

	// Deteccion automatica de cambios y autoguardado recuperable.
	updateSceneDirtyState();
	if (m_sceneDirty) {
		m_autosaveTimer += deltaTime;
		if (m_autosaveTimer >= m_autosaveIntervalSeconds) {
			if (saveSceneInternal(getAutosaveScenePath(), false)) {
				m_autosaveTimer = 0.0f;
				m_recoveryAvailable = true;
				m_gui.m_recoveryAvailable = true;
				m_gui.m_statusMessage = "Autoguardado actualizado";
				MESSAGE("BaseApp", "autosave", "Autoguardado actualizado");
			}
		}
	}
	else {
		m_autosaveTimer = 0.0f;
	}

	m_camera.updateViewMatrix();
	XMStoreFloat4x4(&m_constantBufferStruct.View, XMMatrixTranspose(m_camera.getView()));
	XMStoreFloat4x4(&m_constantBufferStruct.Projection, XMMatrixTranspose(m_camera.getProj()));
	m_constantBufferStruct.CameraPos = m_camera.getPosition();

	if (!m_directionalLightActor.isNull()) {
		EU::TSharedPointer<LightComponent> lightComponent =
			m_directionalLightActor->getComponent<LightComponent>();
		if (lightComponent) {
			const LightData& lightData = lightComponent->getLightData();
			m_constantBufferStruct.LightDir = lightData.direction;
			m_constantBufferStruct.LightColor =
				lightData.color * lightData.intensity;
		}
	}

	m_skybox.update(m_deviceContext, m_camera);
	m_constantBuffer.update(m_deviceContext, nullptr, 0, nullptr, &m_constantBufferStruct, 0, 0);
	m_sceneGraph.update(deltaTime, m_deviceContext);
}

void
BaseApp::render() {
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	m_deviceContext.m_drawCallCount = 0;

	m_renderScene.clear();
	m_sceneGraph.gatherRenderScene(m_renderScene, m_camera);
	m_renderScene.skybox = &m_skybox;

	m_renderPipeline.setShadowFactorDebugEnabled(m_gui.m_visualizeDeferredShadowFactor);
	m_renderPipeline.setDeferredDebugViewMode(m_gui.m_deferredDebugViewMode);
	m_renderPipeline.render(m_deviceContext, m_camera, m_renderScene, m_editorViewportPass);

	m_lastDrawCalls = m_deviceContext.m_drawCallCount;

	m_renderTargetView.render(m_deviceContext, m_depthStencilView, 1, ClearColor);
	m_viewport.render(m_deviceContext);
	m_depthStencilView.render(m_deviceContext);
	m_gui.render();
	m_swapChain.present();
}

void
BaseApp::destroy() {
	if (m_deviceContext.m_deviceContext) m_deviceContext.m_deviceContext->ClearState();
	m_sceneGraph.destroy();
	m_renderPipeline.destroy();
	m_editorViewportPass.destroy();
	m_carRenderMesh.destroy();
	for (Texture& texture : m_carTextures) texture.destroy();
	m_defaultRasterizer.destroy();
	m_defaultDepthStencil.destroy();
	m_defaultSampler.destroy();
	m_shaderProgram.destroy();
	m_depthStencil.destroy();
	m_depthStencilView.destroy();
	m_renderTargetView.destroy();
	m_swapChain.destroy();
	m_backBuffer.destroy();
	if (m_guiInitialized) {
		m_gui.destroy();
		m_guiInitialized = false;
	}
	delete m_carModel;
	m_carModel = nullptr;
	for (auto& lm : m_loadedModels) {
		if (lm) {
			lm->mesh.destroy();
			lm->albedo.destroy(); lm->normal.destroy(); lm->metallic.destroy();
			lm->roughness.destroy(); lm->ao.destroy(); lm->emissive.destroy();
			for (std::unique_ptr<LoadedMaterialResources>& material :
				lm->importedMaterials) {
				if (material) {
					material->destroy();
				}
			}
			lm->importedMaterials.clear();
		}
	}
	m_loadedModels.clear();
	for (auto& tex : m_thumbTextures) tex.destroy();
	m_thumbTextures.clear();

	m_deviceContext.destroy();
	m_device.destroy();
}

LRESULT
BaseApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
		return true;
	}
	switch (message) {
	case WM_CREATE: {
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCreate->lpCreateParams);
	}
				  return 0;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
				 return 0;
	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED) return 0;
		UINT newW = LOWORD(lParam);
		UINT newH = HIWORD(lParam);
		if (newW == 0 || newH == 0) return 0;
		BaseApp* app = reinterpret_cast<BaseApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (app) app->onResize(newW, newH);
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void BaseApp::onResize(unsigned int newW, unsigned int newH)
{
	if (!m_d3dReady) {
		m_window.m_width = (int)newW;
		m_window.m_height = (int)newH;
		return;
	}
	if (!m_deviceContext.m_deviceContext || !m_swapChain.m_swapChain) return;
	if (newW == 0 || newH == 0) return;

	m_window.m_width = (int)newW;
	m_window.m_height = (int)newH;

	ID3D11RenderTargetView* nullRTV = nullptr;
	m_deviceContext.m_deviceContext->OMSetRenderTargets(1, &nullRTV, nullptr);

	m_renderTargetView.destroy();
	m_depthStencilView.destroy();
	m_depthStencil.destroy();
	m_backBuffer.destroy();

	HRESULT hr = m_swapChain.resizeBuffers(newW, newH);
	if (FAILED(hr)) return;
	hr = m_swapChain.getBackBuffer(m_backBuffer);
	if (FAILED(hr)) return;
	hr = m_renderTargetView.init(m_device, m_backBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);
	if (FAILED(hr)) return;
	hr = m_depthStencil.init(m_device, newW, newH, DXGI_FORMAT_D24_UNORM_S8_UINT, D3D11_BIND_DEPTH_STENCIL, 4, 0);
	if (FAILED(hr)) return;
	hr = m_depthStencilView.init(m_device, m_depthStencil, DXGI_FORMAT_D24_UNORM_S8_UINT);
	if (FAILED(hr)) return;

	m_viewport.init(m_window);
	m_camera.setLens(XM_PIDIV4, newW / (float)newH, 0.01f, 100.0f);
}

void BaseApp::handleEditorViewportResize()
{
	if (!m_editorViewportResizePending) return;

	m_deviceContext.m_deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	ID3D11ShaderResourceView* nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
	m_deviceContext.m_deviceContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);

	EditorViewportPass newPass;
	HRESULT hr = newPass.init(m_device, m_pendingViewportWidth, m_pendingViewportHeight);
	if (FAILED(hr)) { m_editorViewportResizePending = false; return; }

	m_editorViewportPass.swap(newPass);
	m_renderPipeline.resize(m_device, m_pendingViewportWidth, m_pendingViewportHeight);
	m_editorViewportResizePending = false;
}

void
BaseApp::captureInitialState() {
	m_initialLightDir = m_constantBufferStruct.LightDir;
	m_initialLightColor = m_constantBufferStruct.LightColor;
	m_initialCameraPos = m_camera.getPosition();
	m_initialTransforms.clear();
	for (auto& actor : m_actors) {
		InitialTransform it{};
		if (!actor.isNull()) {
			EU::TSharedPointer<Transform> t = actor->getComponent<Transform>();
			if (t) { it.position = t->getPosition(); it.rotation = t->getRotation(); it.scale = t->getScale(); }
		}
		m_initialTransforms.push_back(it);
	}
}

void
BaseApp::resetSceneToDefaults() {
	m_constantBufferStruct.LightDir = m_initialLightDir;
	m_constantBufferStruct.LightColor = m_initialLightColor;
	m_camera.setPosition(m_initialCameraPos.x, m_initialCameraPos.y, m_initialCameraPos.z);
	for (size_t i = 0; i < m_actors.size() && i < m_initialTransforms.size(); ++i) {
		if (m_actors[i].isNull()) continue;
		EU::TSharedPointer<Transform> t = m_actors[i]->getComponent<Transform>();
		if (t) {
			t->setPosition(m_initialTransforms[i].position);
			t->setRotation(m_initialTransforms[i].rotation);
			t->setScale(m_initialTransforms[i].scale);
		}
	}
	if (!m_directionalLightActor.isNull()) {
		EU::TSharedPointer<LightComponent> lc = m_directionalLightActor->getComponent<LightComponent>();
		if (lc) { lc->getLightData().direction = m_initialLightDir; lc->getLightData().color = m_initialLightColor; }
	}
	MESSAGE("BaseApp", "resetSceneToDefaults", "Escena restaurada");
}

void
BaseApp::focusCameraOnActor(const EU::TSharedPointer<Actor>& actor) {
	if (actor.isNull()) return;

	EU::TSharedPointer<Transform> transform = actor->getComponent<Transform>();
	if (!transform) return;

	EU::Vector3 localMin, localMax;
	EU::Vector3 target = transform->getPosition();
	float radius = 1.0f;

	if (getActorAABB(actor, localMin, localMax)) {
		float worldMin[3] = { 1e9f, 1e9f, 1e9f };
		float worldMax[3] = { -1e9f, -1e9f, -1e9f };

		for (int corner = 0; corner < 8; ++corner) {
			const float x = (corner & 1) ? localMax.x : localMin.x;
			const float y = (corner & 2) ? localMax.y : localMin.y;
			const float z = (corner & 4) ? localMax.z : localMin.z;

			XMVECTOR worldCorner = XMVector3TransformCoord(
				XMVectorSet(x, y, z, 1.0f),
				transform->worldMatrix);

			XMFLOAT3 value;
			XMStoreFloat3(&value, worldCorner);

			worldMin[0] = fminf(worldMin[0], value.x);
			worldMin[1] = fminf(worldMin[1], value.y);
			worldMin[2] = fminf(worldMin[2], value.z);

			worldMax[0] = fmaxf(worldMax[0], value.x);
			worldMax[1] = fmaxf(worldMax[1], value.y);
			worldMax[2] = fmaxf(worldMax[2], value.z);
		}

		target = EU::Vector3(
			(worldMin[0] + worldMax[0]) * 0.5f,
			(worldMin[1] + worldMax[1]) * 0.5f,
			(worldMin[2] + worldMax[2]) * 0.5f);

		const float dx = worldMax[0] - worldMin[0];
		const float dy = worldMax[1] - worldMin[1];
		const float dz = worldMax[2] - worldMin[2];
		radius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz);
		if (radius < 0.5f) radius = 0.5f;
	}

	const float distance =
		(radius / tanf(m_camera.getFovY() * 0.5f)) * 1.25f;

	EU::Vector3 forward = m_camera.GetForward();
	EU::Vector3 eye(
		target.x - forward.x * distance,
		target.y - forward.y * distance,
		target.z - forward.z * distance);

	m_orbitPivot = target;
	m_orbitDistance = distance;

	m_camera.lookAt(eye, target);
	m_camera.setPosition(eye);
}

void
BaseApp::fitCameraToScene() {
	float minX = 1e9f, minY = 1e9f, minZ = 1e9f;
	float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
	int count = 0;
	for (auto& a : m_actors) {
		if (a.isNull()) continue;
		if (a->getComponent<MeshRendererComponent>().isNull()) continue;
		EU::TSharedPointer<Transform> t = a->getComponent<Transform>();
		if (!t) continue;
		EU::Vector3 p = t->getPosition();
		minX = fminf(minX, p.x); minY = fminf(minY, p.y); minZ = fminf(minZ, p.z);
		maxX = fmaxf(maxX, p.x); maxY = fmaxf(maxY, p.y); maxZ = fmaxf(maxZ, p.z);
		++count;
	}
	if (count == 0) return;
	EU::Vector3 center((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f);
	float dx = maxX - minX, dy = maxY - minY, dz = maxZ - minZ;
	float radius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz) + 3.0f;
	float dist = radius / tanf(m_camera.getFovY() * 0.5f);
	EU::Vector3 fwd = m_camera.GetForward();
	EU::Vector3 eye(center.x - fwd.x * dist, center.y - fwd.y * dist, center.z - fwd.z * dist);
	m_camera.lookAt(eye, center);
	m_camera.setPosition(eye);
}

bool
BaseApp::captureGizmoState(int index, GizmoEditState& out) {
	if (index < 0 || index >= (int)m_actors.size()) return false;
	if (m_actors[index].isNull()) return false;
	EU::TSharedPointer<Transform> t = m_actors[index]->getComponent<Transform>();
	if (!t) return false;
	out.position = t->getPosition();
	out.rotation = t->getRotation();
	out.scale = t->getScale();
	return true;
}

bool
BaseApp::getViewportRay(
    const ImVec2& screenPosition,
    XMFLOAT3& outOrigin,
    XMFLOAT3& outDirection) const {

    const float viewportWidth = m_gui.m_viewportSize.x;
    const float viewportHeight = m_gui.m_viewportSize.y;
    if (viewportWidth < 1.0f || viewportHeight < 1.0f) return false;

    const float mouseX = screenPosition.x - m_gui.m_viewportPos.x;
    const float mouseY = screenPosition.y - m_gui.m_viewportPos.y;
    if (mouseX < 0.0f || mouseY < 0.0f ||
        mouseX > viewportWidth || mouseY > viewportHeight) {
        return false;
    }

    const float ndcX = (2.0f * mouseX / viewportWidth) - 1.0f;
    const float ndcY = 1.0f - (2.0f * mouseY / viewportHeight);
    const XMMATRIX viewProjection = m_camera.getView() * m_camera.getProj();

    const XMVECTOR determinantCheck = XMMatrixDeterminant(viewProjection);
    const float determinantValue = XMVectorGetX(determinantCheck);
    if (!_finite(determinantValue) || fabsf(determinantValue) < 0.000001f) {
        return false;
    }

    XMVECTOR determinant;
    const XMMATRIX inverseViewProjection =
        XMMatrixInverse(&determinant, viewProjection);

    const XMVECTOR nearPoint = XMVector3TransformCoord(
        XMVectorSet(ndcX, ndcY, 0.0f, 1.0f),
        inverseViewProjection);
    const XMVECTOR farPoint = XMVector3TransformCoord(
        XMVectorSet(ndcX, ndcY, 1.0f, 1.0f),
        inverseViewProjection);
    const XMVECTOR direction =
        XMVector3Normalize(XMVectorSubtract(farPoint, nearPoint));

    XMStoreFloat3(&outOrigin, nearPoint);
    XMStoreFloat3(&outDirection, direction);
    return _finite(outDirection.x) &&
        _finite(outDirection.y) &&
        _finite(outDirection.z);
}

bool
BaseApp::getViewportPlacementPosition(
    const ImVec2& screenPosition,
    const EU::Vector3& localMinimum,
    EU::Vector3& outPosition) const {

    XMFLOAT3 rayOrigin{};
    XMFLOAT3 rayDirection{};
    if (!getViewportRay(screenPosition, rayOrigin, rayDirection)) {
        return false;
    }

    float distance = 5.0f;
    if (fabsf(rayDirection.y) > 0.000001f) {
        const float floorDistance = -rayOrigin.y / rayDirection.y;
        if (floorDistance > 0.05f) distance = floorDistance;
    }

    outPosition = EU::Vector3(
        rayOrigin.x + rayDirection.x * distance,
        rayOrigin.y + rayDirection.y * distance - localMinimum.y,
        rayOrigin.z + rayDirection.z * distance);

    if (m_gui.m_snapEnabled && m_gui.m_snapTranslate > 0.000001f) {
        const float snap = m_gui.m_snapTranslate;
        outPosition.x = roundf(outPosition.x / snap) * snap;
        outPosition.y = roundf(outPosition.y / snap) * snap;
        outPosition.z = roundf(outPosition.z / snap) * snap;
    }

    return true;
}

void
BaseApp::pickActorFromMouse() {
	const float viewportX = m_gui.m_viewportPos.x;
	const float viewportY = m_gui.m_viewportPos.y;
	const float viewportWidth = m_gui.m_viewportSize.x;
	const float viewportHeight = m_gui.m_viewportSize.y;

	if (viewportWidth < 1.0f || viewportHeight < 1.0f) return;

	ImVec2 mouse = ImGui::GetIO().MousePos;
	const float mouseX = mouse.x - viewportX;
	const float mouseY = mouse.y - viewportY;

	if (mouseX < 0.0f || mouseY < 0.0f ||
		mouseX > viewportWidth || mouseY > viewportHeight) {
		return;
	}

	const float ndcX = (2.0f * mouseX / viewportWidth) - 1.0f;
	const float ndcY = 1.0f - (2.0f * mouseY / viewportHeight);

	const XMMATRIX viewProjection =
		m_camera.getView() * m_camera.getProj();

	// XNAMath requiere una direccion valida para almacenar
	// el determinante. No se debe enviar nullptr.
	const XMVECTOR determinantCheck =
		XMMatrixDeterminant(viewProjection);

	const float determinantValue =
		XMVectorGetX(determinantCheck);

	if (!_finite(determinantValue) ||
		fabsf(determinantValue) < 0.000001f) {

		ERROR(
			"BaseApp",
			"pickActorFromMouse",
			"La matriz ViewProjection no se puede invertir.");

		return;
	}

	XMVECTOR determinant;

	const XMMATRIX inverseViewProjection =
		XMMatrixInverse(
			&determinant,
			viewProjection);

	XMVECTOR nearPoint = XMVector3TransformCoord(
		XMVectorSet(ndcX, ndcY, 0.0f, 1.0f),
		inverseViewProjection);

	XMVECTOR farPoint = XMVector3TransformCoord(
		XMVectorSet(ndcX, ndcY, 1.0f, 1.0f),
		inverseViewProjection);

	XMVECTOR rayDirectionVector =
		XMVector3Normalize(XMVectorSubtract(farPoint, nearPoint));

	XMFLOAT3 rayOrigin;
	XMFLOAT3 rayDirection;
	XMStoreFloat3(&rayOrigin, nearPoint);
	XMStoreFloat3(&rayDirection, rayDirectionVector);

	float closestDistance = FLT_MAX;
	int closestActorIndex = -1;

	for (int actorIndex = 0;
		actorIndex < static_cast<int>(m_actors.size());
		++actorIndex) {

		EU::TSharedPointer<Actor> actor = m_actors[actorIndex];
		if (actor.isNull()) continue;

		EU::TSharedPointer<MeshRendererComponent> meshRenderer =
			actor->getComponent<MeshRendererComponent>();

		EU::TSharedPointer<Transform> transform =
			actor->getComponent<Transform>();

		if (!meshRenderer || !meshRenderer->isVisible() ||
			!meshRenderer->isSelectable() || !transform)
			continue;

		EU::Vector3 localMin;
		EU::Vector3 localMax;
		if (!getActorAABB(actor, localMin, localMax))
			continue;

		// Broad phase: rayo contra AABB mundial.
		float worldMin[3] = { 1e9f, 1e9f, 1e9f };
		float worldMax[3] = { -1e9f, -1e9f, -1e9f };

		for (int corner = 0; corner < 8; ++corner) {
			const float x = (corner & 1) ? localMax.x : localMin.x;
			const float y = (corner & 2) ? localMax.y : localMin.y;
			const float z = (corner & 4) ? localMax.z : localMin.z;

			XMVECTOR worldCorner = XMVector3TransformCoord(
				XMVectorSet(x, y, z, 1.0f),
				transform->worldMatrix);

			XMFLOAT3 value;
			XMStoreFloat3(&value, worldCorner);

			worldMin[0] = fminf(worldMin[0], value.x);
			worldMin[1] = fminf(worldMin[1], value.y);
			worldMin[2] = fminf(worldMin[2], value.z);

			worldMax[0] = fmaxf(worldMax[0], value.x);
			worldMax[1] = fmaxf(worldMax[1], value.y);
			worldMax[2] = fmaxf(worldMax[2], value.z);
		}

		const float origin[3] = {
			rayOrigin.x, rayOrigin.y, rayOrigin.z
		};

		const float direction[3] = {
			rayDirection.x, rayDirection.y, rayDirection.z
		};

		float aabbNear = 0.0f;
		float aabbFar = FLT_MAX;
		bool aabbHit = true;

		for (int axis = 0; axis < 3; ++axis) {
			if (fabsf(direction[axis]) < 1e-8f) {
				if (origin[axis] < worldMin[axis] ||
					origin[axis] > worldMax[axis]) {
					aabbHit = false;
					break;
				}
			}
			else {
				const float inverseDirection = 1.0f / direction[axis];
				float first =
					(worldMin[axis] - origin[axis]) * inverseDirection;
				float second =
					(worldMax[axis] - origin[axis]) * inverseDirection;

				if (first > second) {
					const float temporary = first;
					first = second;
					second = temporary;
				}

				aabbNear = fmaxf(aabbNear, first);
				aabbFar = fminf(aabbFar, second);

				if (aabbNear > aabbFar) {
					aabbHit = false;
					break;
				}
			}
		}

		if (!aabbHit || aabbNear > closestDistance)
			continue;

		// Narrow phase: rayo contra triángulos CPU.
		const std::vector<MeshComponent>* cpuMeshes =
			getActorCpuMeshes(actor);

		bool preciseHit = false;
		float preciseDistance = FLT_MAX;

		if (cpuMeshes) {
			for (const MeshComponent& mesh : *cpuMeshes) {
				for (size_t index = 0;
					index + 2 < mesh.m_index.size();
					index += 3) {

					const unsigned int i0 = mesh.m_index[index + 0];
					const unsigned int i1 = mesh.m_index[index + 1];
					const unsigned int i2 = mesh.m_index[index + 2];

					if (i0 >= mesh.m_vertex.size() ||
						i1 >= mesh.m_vertex.size() ||
						i2 >= mesh.m_vertex.size()) {
						continue;
					}

					const SimpleVertex& vertex0 = mesh.m_vertex[i0];
					const SimpleVertex& vertex1 = mesh.m_vertex[i1];
					const SimpleVertex& vertex2 = mesh.m_vertex[i2];

					XMVECTOR worldVertex0 = XMVector3TransformCoord(
						XMVectorSet(
							vertex0.Position.x,
							vertex0.Position.y,
							vertex0.Position.z,
							1.0f),
						transform->worldMatrix);

					XMVECTOR worldVertex1 = XMVector3TransformCoord(
						XMVectorSet(
							vertex1.Position.x,
							vertex1.Position.y,
							vertex1.Position.z,
							1.0f),
						transform->worldMatrix);

					XMVECTOR worldVertex2 = XMVector3TransformCoord(
						XMVectorSet(
							vertex2.Position.x,
							vertex2.Position.y,
							vertex2.Position.z,
							1.0f),
						transform->worldMatrix);

					XMFLOAT3 triangle0;
					XMFLOAT3 triangle1;
					XMFLOAT3 triangle2;
					XMStoreFloat3(&triangle0, worldVertex0);
					XMStoreFloat3(&triangle1, worldVertex1);
					XMStoreFloat3(&triangle2, worldVertex2);

					float triangleDistance = 0.0f;
					if (rayTriangleIntersection(
						rayOrigin,
						rayDirection,
						triangle0,
						triangle1,
						triangle2,
						triangleDistance)) {

						preciseHit = true;
						preciseDistance =
							fminf(preciseDistance, triangleDistance);
					}
				}
			}
		}

		// Si no hay copia CPU, se conserva el fallback AABB.
		const float actorDistance =
			preciseHit ? preciseDistance : aabbNear;

		if (actorDistance < closestDistance) {
			closestDistance = actorDistance;
			closestActorIndex = actorIndex;
		}
	}

	if (closestActorIndex >= 0) {
		m_gui.selectedActorIndex = closestActorIndex;
		MESSAGE("BaseApp", "pickActorFromMouse", "Actor seleccionado desde viewport");
	}
	else {
		m_gui.selectedActorIndex = -1;
	}
}

void
BaseApp::addActorToScene(const EU::TSharedPointer<Actor>& actor) {
	if (actor.isNull()) return;

	for (const auto& existing : m_actors) {
		if (!existing.isNull() && existing.get() == actor.get()) {
			return;
		}
	}

	actor->setName(makeUniqueActorName(actor->getName(), actor.get()));
	m_actors.push_back(actor);
	m_sceneGraph.addEntity(actor.get());
	m_sceneGraph.validateHierarchy(true);
}

bool
BaseApp::reparentActor(
	const EU::TSharedPointer<Actor>& child,
	const EU::TSharedPointer<Actor>& parent,
	bool keepWorldTransform) {
	if (child.isNull()) return false;
	if (findActorShared(child.get()).isNull()) return false;
	if (!parent.isNull() && child.get() == parent.get()) return false;
	if (!parent.isNull() && findActorShared(parent.get()).isNull()) return false;

	return m_sceneGraph.reparent(
		child.get(),
		parent.isNull() ? nullptr : parent.get(),
		keepWorldTransform);
}

void
BaseApp::removeActorFromScene(const EU::TSharedPointer<Actor>& actor) {
	if (actor.isNull()) return;

	int removedIndex = -1;
	for (int index = 0; index < static_cast<int>(m_actors.size()); ++index) {
		if (!m_actors[index].isNull() &&
			m_actors[index].get() == actor.get()) {
			removedIndex = index;
			break;
		}
	}
	if (removedIndex < 0) return;

	Actor* parentRaw = dynamic_cast<Actor*>(
		m_sceneGraph.getParent(actor.get()));
	EU::TSharedPointer<Actor> parentActor = findActorShared(parentRaw);
	const bool removedWasSelected =
		m_gui.selectedActorIndex == removedIndex;

	if (m_directionalLightActor.get() == actor.get()) {
		m_directionalLightActor.reset();
	}
	if (m_lastSelectedRenderableActor.get() == actor.get()) {
		m_lastSelectedRenderableActor.reset();
	}

	m_sceneGraph.removeEntity(actor.get(), true);
	m_actors.erase(m_actors.begin() + removedIndex);

	if (removedWasSelected) {
		m_gui.selectedActorIndex = -1;
		if (!parentActor.isNull()) {
			for (int index = 0;
				index < static_cast<int>(m_actors.size());
				++index) {
				if (!m_actors[index].isNull() &&
					m_actors[index].get() == parentActor.get()) {
					m_gui.selectedActorIndex = index;
					break;
				}
			}
		}
	}
	else if (m_gui.selectedActorIndex > removedIndex) {
		--m_gui.selectedActorIndex;
	}

	if (m_gui.selectedActorIndex >= static_cast<int>(m_actors.size())) {
		m_gui.selectedActorIndex = m_actors.empty()
			? -1
			: static_cast<int>(m_actors.size()) - 1;
	}
}


EU::TSharedPointer<Actor>
BaseApp::spawnCar(
	const std::string& name,
	const EU::Vector3& pos,
	const EU::Vector3& rot,
	const EU::Vector3& scale) {

	EU::TSharedPointer<Actor> actor =
		EU::MakeShared<Actor>(m_device);
	if (actor.isNull())
		return actor;

	actor->setName(name);

	EU::TSharedPointer<Transform> transform =
		actor->getComponent<Transform>();
	if (transform)
		transform->setTransform(pos, rot, scale);

	EU::TSharedPointer<MeshRendererComponent> meshRenderer =
		actor->getComponent<MeshRendererComponent>();
	if (!meshRenderer) {
		meshRenderer =
			EU::MakeShared<MeshRendererComponent>();
		actor->addComponent(meshRenderer);
	}

	std::vector<MaterialInstance*> materialPointers;
	materialPointers.reserve(kCarMaterialCount);
	for (size_t materialIndex = 0;
		materialIndex < kCarMaterialCount;
		++materialIndex) {
		materialPointers.push_back(
			&m_carMaterials[materialIndex]);
	}

	meshRenderer->setMesh(&m_carRenderMesh);
	meshRenderer->setMaterialInstances(materialPointers);
	meshRenderer->setVisible(true);
	meshRenderer->setCastShadow(true);
	return actor;
}


EU::TSharedPointer<Actor>
BaseApp::spawnActorFromSource(
	const std::string& modelPath,
	const std::string& name,
	const EU::Vector3& position,
	const EU::Vector3& rotation,
	const EU::Vector3& scale) {

	EU::TSharedPointer<Actor> actor;

	if (modelPath.empty() ||
		toLowerCopy(modelPath) == toLowerCopy("Assets/Models/AlfaRomeo33Stradale.fbx")) {
		actor = spawnCar(name, position, rotation, scale);
		if (!actor.isNull())
			m_actorSourcePaths[actor.get()] = "Assets/Models/AlfaRomeo33Stradale.fbx";
	}
	else {
		actor = loadModelActor(modelPath);
		if (!actor.isNull()) {
			actor->setName(name);
			EU::TSharedPointer<Transform> transform =
				actor->getComponent<Transform>();

			if (transform)
				transform->setTransform(position, rotation, scale);

			m_actorSourcePaths[actor.get()] = modelPath;
		}
	}

	return actor;
}

const std::vector<MeshComponent>*
BaseApp::getActorCpuMeshes(
	const EU::TSharedPointer<Actor>& actor) const {

	if (actor.isNull()) return nullptr;

	EU::TSharedPointer<MeshRendererComponent> meshRenderer =
		actor->getComponent<MeshRendererComponent>();

	if (!meshRenderer || !meshRenderer->getMesh())
		return nullptr;

	Mesh* mesh = meshRenderer->getMesh();

	if (mesh == &m_carRenderMesh)
		return &m_carCpuMeshes;

	for (const auto& loadedModel : m_loadedModels) {
		if (loadedModel && &loadedModel->mesh == mesh)
			return &loadedModel->cpuMeshes;
	}

	return nullptr;
}

void
BaseApp::duplicateSelected() {
	const int index = m_gui.selectedActorIndex;
	if (index < 0 ||
		index >= static_cast<int>(m_actors.size()) ||
		m_actors[index].isNull()) {
		MESSAGE("BaseApp", "duplicateSelected", "No hay actor seleccionado");
		return;
	}

	EU::TSharedPointer<Actor> source = m_actors[index];
	EU::TSharedPointer<Transform> transform =
		source->getComponent<Transform>();
	if (!transform) return;

	EU::Vector3 position = transform->getPosition();
	position.x += 1.5f;

	Actor* parentRaw = dynamic_cast<Actor*>(
		m_sceneGraph.getParent(source.get()));
	EU::TSharedPointer<Actor> parent = findActorShared(parentRaw);

	EU::TSharedPointer<Actor> duplicated = cloneActor(
		source,
		source->getName() + "_copy",
		position);
	if (duplicated.isNull()) {
		ERROR("BaseApp", "duplicateSelected", "No se pudo duplicar el actor");
		return;
	}

	addActorToScene(duplicated);
	if (!parent.isNull()) {
		reparentActor(duplicated, parent, false);
	}

	m_commands.push(std::unique_ptr<ICommand>(
		new SpawnActorCommand(this, duplicated, parent)));
	m_gui.selectedActorIndex = static_cast<int>(m_actors.size()) - 1;
	MESSAGE("BaseApp", "duplicateSelected", "Actor duplicado");
}

void
BaseApp::deleteSelected() {
	const int index = m_gui.selectedActorIndex;
	if (index < 0 ||
		index >= static_cast<int>(m_actors.size()) ||
		m_actors[index].isNull()) {
		MESSAGE("BaseApp", "deleteSelected", "No hay actor seleccionado");
		return;
	}

	EU::TSharedPointer<Actor> actor = m_actors[index];
	Actor* parentRaw = dynamic_cast<Actor*>(
		m_sceneGraph.getParent(actor.get()));
	EU::TSharedPointer<Actor> parent = findActorShared(parentRaw);

	std::vector<EU::TSharedPointer<Actor>> children;
	const std::vector<Entity*>* childEntities =
		m_sceneGraph.getChildren(actor.get());
	if (childEntities) {
		children.reserve(childEntities->size());
		for (Entity* childEntity : *childEntities) {
			Actor* childActor = dynamic_cast<Actor*>(childEntity);
			EU::TSharedPointer<Actor> child = findActorShared(childActor);
			if (!child.isNull()) children.push_back(child);
		}
	}

	removeActorFromScene(actor);
	m_commands.push(std::unique_ptr<ICommand>(
		new DeleteActorCommand(this, actor, parent, children)));
	MESSAGE("BaseApp", "deleteSelected", "Actor eliminado");
}

void
BaseApp::copySelected() {
	const int index = m_gui.selectedActorIndex;
	if (index < 0 ||
		index >= static_cast<int>(m_actors.size()) ||
		m_actors[index].isNull()) {
		MESSAGE("BaseApp", "copySelected", "No hay actor seleccionado");
		return;
	}

	EU::TSharedPointer<Actor> source = m_actors[index];
	EU::TSharedPointer<Transform> transform =
		source->getComponent<Transform>();
	if (!transform) return;

	m_clipboard = ActorClipboard();
	m_clipboard.sourceActor = source;
	m_clipboard.sourceParent = findActorShared(dynamic_cast<Actor*>(
		m_sceneGraph.getParent(source.get())));
	m_clipboard.name = source->getName();
	m_clipboard.position = transform->getPosition();
	m_clipboard.rotation = transform->getRotation();
	m_clipboard.scale = transform->getScale();
	m_clipboard.actorActive = source->isActive();
	m_clipboard.castShadow = source->canCastShadow();

	EU::TSharedPointer<LightComponent> light =
		source->getComponent<LightComponent>();
	EU::TSharedPointer<MeshRendererComponent> meshRenderer =
		source->getComponent<MeshRendererComponent>();

	if (light) {
		m_clipboard.kind = ActorClipboardKind::Light;
		m_clipboard.lightData = light->getLightData();
		m_clipboard.followLightPosition =
			light->followsTransformPosition();
		m_clipboard.followLightDirection =
			light->followsTransformDirection();
	}
	else if (meshRenderer && meshRenderer->hasMesh()) {
		m_clipboard.kind = ActorClipboardKind::Model;
		m_clipboard.visible = meshRenderer->isVisible();
		m_clipboard.castShadow = meshRenderer->canCastShadow();
		m_clipboard.receiveShadow = meshRenderer->canReceiveShadow();
		m_clipboard.selectable = meshRenderer->isSelectable();

		const auto sourceIterator = m_actorSourcePaths.find(source.get());
		if (sourceIterator != m_actorSourcePaths.end()) {
			m_clipboard.modelPath = sourceIterator->second;
		}
	}
	else {
		m_clipboard.kind = ActorClipboardKind::Empty;
	}

	m_hasClipboard = true;
	MESSAGE("BaseApp", "copySelected", "Actor copiado al portapapeles");
}

void
BaseApp::pasteClipboard() {
	if (!m_hasClipboard) {
		MESSAGE("BaseApp", "pasteClipboard", "Portapapeles vacio");
		return;
	}

	EU::Vector3 position = m_clipboard.position;
	position.x += 1.5f;

	EU::TSharedPointer<Actor> pasted;
	if (!m_clipboard.sourceActor.isNull()) {
		pasted = cloneActor(
			m_clipboard.sourceActor,
			m_clipboard.name + "_paste",
			position);
	}
	else if (m_clipboard.kind == ActorClipboardKind::Light) {
		pasted = spawnLightActor(
			m_clipboard.lightData.type,
			m_clipboard.name + "_paste",
			position,
			m_clipboard.lightData.color,
			m_clipboard.lightData.intensity,
			m_clipboard.lightData.range,
			m_clipboard.lightData.castShadow);
	}
	else if (m_clipboard.kind == ActorClipboardKind::Model) {
		pasted = spawnActorFromSource(
			m_clipboard.modelPath,
			m_clipboard.name + "_paste",
			position,
			m_clipboard.rotation,
			m_clipboard.scale);
	}
	else {
		pasted = EU::MakeShared<Actor>(m_device);
		if (!pasted.isNull()) {
			pasted->setName(m_clipboard.name + "_paste");
			EU::TSharedPointer<Transform> transform =
				pasted->getComponent<Transform>();
			if (transform) {
				transform->setTransform(
					position,
					m_clipboard.rotation,
					m_clipboard.scale);
			}
		}
	}

	if (pasted.isNull()) {
		ERROR("BaseApp", "pasteClipboard", "No se pudo pegar el actor");
		return;
	}

	pasted->setActive(m_clipboard.actorActive);
	EU::TSharedPointer<Transform> pastedTransform =
		pasted->getComponent<Transform>();
	if (pastedTransform) {
		pastedTransform->setTransform(
			position,
			m_clipboard.rotation,
			m_clipboard.scale);
	}

	EU::TSharedPointer<MeshRendererComponent> pastedRenderer =
		pasted->getComponent<MeshRendererComponent>();
	if (pastedRenderer) {
		pastedRenderer->setVisible(m_clipboard.visible);
		pastedRenderer->setCastShadow(m_clipboard.castShadow);
		pastedRenderer->setReceiveShadow(m_clipboard.receiveShadow);
		pastedRenderer->setSelectable(m_clipboard.selectable);
	}

	EU::TSharedPointer<LightComponent> pastedLight =
		pasted->getComponent<LightComponent>();
	if (pastedLight && m_clipboard.kind == ActorClipboardKind::Light) {
		const LightData& lightData = m_clipboard.lightData;
		pastedLight->setType(lightData.type);
		pastedLight->setColor(lightData.color);
		pastedLight->setIntensity(lightData.intensity);
		pastedLight->setRange(lightData.range);
		pastedLight->setDirection(lightData.direction);
		pastedLight->setSpotAngles(
			lightData.innerSpotAngle,
			lightData.spotAngle);
		pastedLight->setCastShadow(lightData.castShadow);
		pastedLight->setEnabled(lightData.enabled);
		pastedLight->setFollowTransformPosition(
			m_clipboard.followLightPosition);
		pastedLight->setFollowTransformDirection(
			m_clipboard.followLightDirection);
	}

	addActorToScene(pasted);
	if (!m_clipboard.sourceParent.isNull()) {
		reparentActor(pasted, m_clipboard.sourceParent, false);
	}

	m_commands.push(std::unique_ptr<ICommand>(
		new SpawnActorCommand(
			this,
			pasted,
			m_clipboard.sourceParent)));
	m_gui.selectedActorIndex = static_cast<int>(m_actors.size()) - 1;
	MESSAGE("BaseApp", "pasteClipboard", "Actor pegado");
}


void
BaseApp::savePrefabSelected() {
	const int index = m_gui.selectedActorIndex;

	if (index < 0 ||
		index >= static_cast<int>(m_actors.size()) ||
		m_actors[index].isNull()) {
		MESSAGE("BaseApp", "savePrefabSelected", "No hay actor seleccionado");
		return;
	}

	EU::TSharedPointer<Actor> source = m_actors[index];
	EU::TSharedPointer<Transform> transform =
		source->getComponent<Transform>();

	if (!transform) return;

	CreateDirectoryA("Saved", nullptr);

	std::ofstream file("Saved/actor.prefab", std::ios::trunc);
	if (!file.is_open()) {
		ERROR("BaseApp", "savePrefabSelected", "No se pudo guardar el prefab");
		return;
	}

	std::string modelPath;
	auto sourceIterator = m_actorSourcePaths.find(source.get());
	if (sourceIterator != m_actorSourcePaths.end())
		modelPath = sourceIterator->second;

	EU::Vector3 position = transform->getPosition();
	EU::Vector3 rotation = transform->getRotation();
	EU::Vector3 scale = transform->getScale();

	bool visible = true;
	bool castShadow = true;

	EU::TSharedPointer<MeshRendererComponent> meshRenderer =
		source->getComponent<MeshRendererComponent>();

	if (meshRenderer) {
		visible = meshRenderer->isVisible();
		castShadow = meshRenderer->canCastShadow();
	}

	file << "PREFAB 2\n";
	file << "NAME " << std::quoted(source->getName()) << "\n";
	file << "MODEL " << std::quoted(modelPath) << "\n";
	file << "POSITION "
		<< position.x << " " << position.y << " " << position.z << "\n";
	file << "ROTATION "
		<< rotation.x << " " << rotation.y << " " << rotation.z << "\n";
	file << "SCALE "
		<< scale.x << " " << scale.y << " " << scale.z << "\n";
	file << "VISIBLE " << (visible ? 1 : 0) << "\n";
	file << "CAST_SHADOW " << (castShadow ? 1 : 0) << "\n";

	MESSAGE(
		"BaseApp",
		"savePrefabSelected",
		"Prefab guardado en Saved/actor.prefab");
}

void
BaseApp::loadPrefab() {
	std::ifstream file("Saved/actor.prefab");

	if (!file.is_open()) {
		ERROR("BaseApp", "loadPrefab", "No existe Saved/actor.prefab");
		return;
	}

	std::string token;
	std::string name = "Prefab";
	std::string modelPath;

	EU::Vector3 position(0.0f, 0.0f, 0.0f);
	EU::Vector3 rotation(0.0f, 0.0f, 0.0f);
	EU::Vector3 scale(1.0f, 1.0f, 1.0f);

	bool visible = true;
	bool castShadow = true;
	int version = 0;

	file >> token >> version;

	while (file >> token) {
		if (token == "NAME")
			file >> std::quoted(name);
		else if (token == "MODEL")
			file >> std::quoted(modelPath);
		else if (token == "POSITION")
			file >> position.x >> position.y >> position.z;
		else if (token == "ROTATION")
			file >> rotation.x >> rotation.y >> rotation.z;
		else if (token == "SCALE")
			file >> scale.x >> scale.y >> scale.z;
		else if (token == "VISIBLE") {
			int value = 1;
			file >> value;
			visible = value != 0;
		}
		else if (token == "CAST_SHADOW") {
			int value = 1;
			file >> value;
			castShadow = value != 0;
		}
	}

	EU::TSharedPointer<Actor> actor = spawnActorFromSource(
		modelPath,
		name,
		position,
		rotation,
		scale);

	if (actor.isNull()) {
		ERROR("BaseApp", "loadPrefab", "No se pudo crear el prefab");
		return;
	}

	EU::TSharedPointer<MeshRendererComponent> meshRenderer =
		actor->getComponent<MeshRendererComponent>();

	if (meshRenderer) {
		meshRenderer->setVisible(visible);
		meshRenderer->setCastShadow(castShadow);
	}

	addActorToScene(actor);
	m_commands.push(std::unique_ptr<ICommand>(
		new SpawnActorCommand(this, actor)));

	m_gui.selectedActorIndex =
		static_cast<int>(m_actors.size()) - 1;

	MESSAGE("BaseApp", "loadPrefab", "Prefab cargado");
}

void
BaseApp::buildTextureThumbnails() {
	m_thumbTextures.clear();
	m_thumbnails.clear();
	m_thumbTextures.reserve(64);

	std::vector<std::string> folders = listSubfolders("Assets/Textures");
	folders.push_back(""); // tambien la raiz
	int loaded = 0;
	for (const std::string& sub : folders) {
		std::string dir = sub.empty() ? "Assets/Textures" : ("Assets/Textures/" + sub);
		std::vector<std::string> files = listImageFiles(dir);
		for (const std::string& f : files) {
			if (loaded >= 48) break;
			std::string base = dir + "/" + stripExt(f);
			ExtensionType ext = extFromName(toLowerCopy(f));
			Texture tex;
			if (SUCCEEDED(tex.init(m_device, base, ext))) {
				m_thumbTextures.push_back(tex);
				AssetThumb th; th.name = f; th.srv = m_thumbTextures.back().m_textureFromImg;
				m_thumbnails.push_back(th);
				++loaded;
			}
		}
	}
	MESSAGE("BaseApp", "buildTextureThumbnails", "Thumbnails de texturas cargados");
}

void
BaseApp::loadModelTextures(
	LoadedModel& loadedModel,
	const Model3D& importedModel,
	const std::string& modelPath) {

	loadedModel.importedMaterials.clear();

	const std::string modelName =
		fileBaseName(modelPath);
	const std::string modelDirectory =
		directoryOfPath(modelPath);

	std::vector<std::string> searchRoots;

	auto addSearchRoot =
		[&searchRoots](const std::string& root) {
			if (root.empty()) {
				return;
			}

			const std::string lowerRoot =
				toLowerCopy(root);

			for (const std::string& existing :
				searchRoots) {
				if (toLowerCopy(existing) ==
					lowerRoot) {
					return;
				}
			}

			searchRoots.push_back(root);
		};

	addSearchRoot(modelDirectory);
	addSearchRoot(
		joinPath(
			modelDirectory,
			"Textures"));
	addSearchRoot(
		joinPath(
			modelDirectory,
			"textures"));
	addSearchRoot(
		joinPath(
			modelDirectory,
			"Materials"));
	addSearchRoot(
		joinPath(
			"Assets/Textures",
			modelName));
	addSearchRoot("Assets/Textures");

	std::vector<std::string> indexedImages;

	for (size_t rootIndex = 0;
		rootIndex < searchRoots.size();
		++rootIndex) {

		// La carpeta global se limita para no recorrer un proyecto enorme.
		const int depth =
			toLowerCopy(searchRoots[rootIndex]) ==
				toLowerCopy("Assets/Textures")
			? 4
			: 4;

		collectImagePathsRecursive(
			searchRoots[rootIndex],
			depth,
			indexedImages);
	}

	std::sort(
		indexedImages.begin(),
		indexedImages.end());

	indexedImages.erase(
		std::unique(
			indexedImages.begin(),
			indexedImages.end()),
		indexedImages.end());

	std::vector<ImportedMaterialInfo>
		importedMaterials =
			importedModel.GetImportedMaterials();

	if (importedMaterials.empty()) {
		importedMaterials.push_back(
			ImportedMaterialInfo());
	}

	auto resolveTexture =
		[&](const std::string& reference,
			TextureSemantic semantic,
			const std::string& materialName) {

			std::string resolved =
				resolveReferencedTexturePath(
					reference,
					modelPath,
					searchRoots,
					indexedImages);

			if (resolved.empty()) {
				resolved =
					findTextureBySemantic(
						semantic,
						materialName,
						modelName,
						indexedImages);
			}

			return resolved;
		};

	auto loadTexture =
		[this](Texture& destination,
			const std::string& path) {

			if (path.empty() ||
				!filePathExists(path)) {
				return false;
			}

			const ExtensionType extension =
				extFromName(
					toLowerCopy(path));

			const HRESULT result =
				destination.init(
					m_device,
					path,
					extension);

			return SUCCEEDED(result);
		};

	for (size_t materialIndex = 0;
		materialIndex <
			importedMaterials.size();
		++materialIndex) {

		const ImportedMaterialInfo& imported =
			importedMaterials[materialIndex];

		std::unique_ptr<LoadedMaterialResources>
			resources(
				new LoadedMaterialResources());

		resources->name =
			imported.name.empty()
			? ("Material " +
				std::to_string(materialIndex))
			: imported.name;

		const std::string albedoPath =
			resolveTexture(
				imported.albedoTexture,
				TextureSemantic::Albedo,
				resources->name);

		const std::string normalPath =
			resolveTexture(
				imported.normalTexture,
				TextureSemantic::Normal,
				resources->name);

		const std::string metallicPath =
			resolveTexture(
				imported.metallicTexture,
				TextureSemantic::Metallic,
				resources->name);

		const std::string roughnessPath =
			resolveTexture(
				imported.roughnessTexture,
				TextureSemantic::Roughness,
				resources->name);

		const std::string aoPath =
			resolveTexture(
				imported.aoTexture,
				TextureSemantic::AO,
				resources->name);

		const std::string emissivePath =
			resolveTexture(
				imported.emissiveTexture,
				TextureSemantic::Emissive,
				resources->name);

		const bool hasAlbedo =
			loadTexture(
				resources->albedo,
				albedoPath);
		const bool hasNormal =
			loadTexture(
				resources->normal,
				normalPath);
		const bool hasMetallic =
			loadTexture(
				resources->metallic,
				metallicPath);
		const bool hasRoughness =
			loadTexture(
				resources->roughness,
				roughnessPath);
		const bool hasAO =
			loadTexture(
				resources->ao,
				aoPath);
		const bool hasEmissive =
			loadTexture(
				resources->emissive,
				emissivePath);

		const std::string lowerMaterialName =
			toLowerCopy(resources->name);

		const bool transparent =
			imported.opacity < 0.98f ||
			containsStr(
				lowerMaterialName,
				"glass") ||
			containsStr(
				lowerMaterialName,
				"window") ||
			containsStr(
				lowerMaterialName,
				"transparent") ||
			containsStr(
				lowerMaterialName,
				"lens");

		resources->materialInstance.setMaterial(
			transparent
			? &m_transparentPbrMaterial
			: &m_pbrMaterial);

		resources->materialInstance.setAlbedo(
			hasAlbedo
			? &resources->albedo
			: &m_carTextures[CarTexWhite]);

		resources->materialInstance.setNormal(
			hasNormal
			? &resources->normal
			: &m_carTextures[CarTexFlatNormal]);

		// Para valores constantes se usa una textura blanca y el escalar
		// del material. Esto evita que un fallback negro anule el valor PBR.
		resources->materialInstance.setMetallic(
			hasMetallic
			? &resources->metallic
			: &m_carTextures[CarTexMetallicWhite]);

		resources->materialInstance.setRoughness(
			hasRoughness
			? &resources->roughness
			: &m_carTextures[CarTexMetallicWhite]);

		resources->materialInstance.setAO(
			hasAO
			? &resources->ao
			: &m_carTextures[CarTexWhite]);

		resources->materialInstance.setEmissive(
			hasEmissive
			? &resources->emissive
			: &m_carTextures[CarTexEmissiveBlack]);

		MaterialParams& parameters =
			resources->materialInstance.getParams();

		auto clamp01 = [](float value, float fallback) {
			if (!std::isfinite(value)) {
				return fallback;
			}
			return (std::max)(0.0f, (std::min)(1.0f, value));
		};

		const bool intentionallyDark =
			containsStr(lowerMaterialName, "black") ||
			containsStr(lowerMaterialName, "tire") ||
			containsStr(lowerMaterialName, "rubber") ||
			containsStr(lowerMaterialName, "undercarriage") ||
			containsStr(lowerMaterialName, "shadow");

		XMFLOAT4 safeBaseColor(
			clamp01(imported.baseColor.x, 1.0f),
			clamp01(imported.baseColor.y, 1.0f),
			clamp01(imported.baseColor.z, 1.0f),
			clamp01(imported.opacity, 1.0f));

		// Una textura de albedo ya contiene el color. Multiplicarla por el
		// Diffuse del FBX (que frecuentemente es negro) oscurecia todo.
		if (hasAlbedo) {
			safeBaseColor.x = 1.0f;
			safeBaseColor.y = 1.0f;
			safeBaseColor.z = 1.0f;
		}
		else {
			const float luminance =
				safeBaseColor.x * 0.2126f +
				safeBaseColor.y * 0.7152f +
				safeBaseColor.z * 0.0722f;

			if (luminance < 0.025f && !intentionallyDark) {
				safeBaseColor.x = 0.72f;
				safeBaseColor.y = 0.72f;
				safeBaseColor.z = 0.72f;
			}
		}

		parameters.baseColor = safeBaseColor;
		parameters.metallic = hasMetallic
			? 1.0f
			: clamp01(imported.metallic, 0.0f);
		parameters.roughness = hasRoughness
			? 1.0f
			: (std::max)(
				0.05f,
				clamp01(imported.roughness, 0.55f));
		parameters.ao = 1.0f;
		parameters.normalScale = hasNormal ? 1.0f : 0.0f;
		parameters.emissiveStrength = hasEmissive ? 1.0f : 0.0f;
		parameters.alphaCutoff = transparent ? 0.0f : 0.5f;

		MESSAGE(
			"BaseApp",
			"loadModelTextures",
			L"Material importado: "
			<< std::wstring(
				resources->name.begin(),
				resources->name.end())
			<< L" | Albedo: "
			<< (hasAlbedo
				? L"OK"
				: L"fallback")
			<< L" | Normal: "
			<< (hasNormal
				? L"OK"
				: L"fallback"));

		loadedModel.importedMaterials.push_back(
			std::move(resources));
	}

	if (!loadedModel.importedMaterials.empty()) {
		loadedModel.materialInstance =
			loadedModel.importedMaterials[0]->
				materialInstance;
	}
}

EU::TSharedPointer<Actor>
BaseApp::loadModelActor(const std::string& modelPath) {
	std::string lower = toLowerCopy(modelPath);
	ModelType type = FBX;
	if (endsWith(lower, ".obj")) type = OBJ;

	Model3D model(modelPath, type);
	const std::vector<MeshComponent>& meshes = model.GetMeshes();
	if (meshes.empty()) {
		ERROR("BaseApp", "loadModelActor", "El modelo no tiene mallas");
		return EU::TSharedPointer<Actor>();
	}

	std::unique_ptr<LoadedModel> lm(new LoadedModel());
	lm->cpuMeshes = meshes;
	lm->sourcePath = modelPath;

	HRESULT hr;
	for (const MeshComponent& mc : meshes) {
		Submesh sm{};
		hr = sm.vertexBuffer.init(m_device, mc, D3D11_BIND_VERTEX_BUFFER);
		if (FAILED(hr)) { ERROR("BaseApp", "loadModelActor", "Fallo vertex buffer"); return EU::TSharedPointer<Actor>(); }
		hr = sm.indexBuffer.init(m_device, mc, D3D11_BIND_INDEX_BUFFER);
		if (FAILED(hr)) { ERROR("BaseApp", "loadModelActor", "Fallo index buffer"); return EU::TSharedPointer<Actor>(); }
		sm.indexCount = mc.m_numIndex;
		sm.startIndex = 0;
		sm.materialSlot = mc.m_materialSlot;
		lm->mesh.getSubmeshes().push_back(std::move(sm));
	}
	lm->localMin = EU::Vector3(1e9f, 1e9f, 1e9f);
	lm->localMax = EU::Vector3(-1e9f, -1e9f, -1e9f);
	for (const MeshComponent& mc : meshes) {
		for (const SimpleVertex& v : mc.m_vertex) {
			lm->localMin.x = fminf(lm->localMin.x, v.Position.x);
			lm->localMin.y = fminf(lm->localMin.y, v.Position.y);
			lm->localMin.z = fminf(lm->localMin.z, v.Position.z);
			lm->localMax.x = fmaxf(lm->localMax.x, v.Position.x);
			lm->localMax.y = fmaxf(lm->localMax.y, v.Position.y);
			lm->localMax.z = fmaxf(lm->localMax.z, v.Position.z);
		}
	}



	lm->material.setShader(&m_shaderProgram);
	lm->material.setRasterizerState(&m_defaultRasterizer);
	lm->material.setDepthStencilState(&m_defaultDepthStencil);
	lm->material.setSamplerState(&m_defaultSampler);
	lm->material.setDomain(MaterialDomain::Opaque);
	lm->material.setBlendMode(BlendMode::Opaque);

	lm->materialInstance.setMaterial(&lm->material);
	lm->materialInstance.getParams().baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lm->materialInstance.getParams().metallic = 0.0f;
	lm->materialInstance.getParams().roughness = 0.55f;
	lm->materialInstance.getParams().ao = 1.0f;
	lm->materialInstance.getParams().normalScale = 0.0f;
	lm->materialInstance.getParams().emissiveStrength = 0.0f;
	lm->materialInstance.getParams().alphaCutoff = 0.5f;

	std::string modelName = fileBaseName(modelPath);
	loadModelTextures(
		*lm,
		model,
		modelPath);

	EU::TSharedPointer<Actor> a = EU::MakeShared<Actor>(m_device);
	if (a.isNull()) return a;
	a->setName(modelName);
	EU::TSharedPointer<Transform> t = a->getComponent<Transform>();
	if (t) t->setTransform(EU::Vector3(0.0f, 2.92f, 5.60f), EU::Vector3(0.0f, 0.0f, 0.0f), EU::Vector3(1.0f, 1.0f, 1.0f));
	EU::TSharedPointer<MeshRendererComponent> mr = a->getComponent<MeshRendererComponent>();
	if (!mr) { mr = EU::MakeShared<MeshRendererComponent>(); a->addComponent(mr); }
	mr->setMesh(&lm->mesh);

	std::vector<MaterialInstance*> materialPointers;
	materialPointers.reserve(
		lm->importedMaterials.size());

	for (const std::unique_ptr<LoadedMaterialResources>&
		materialResources :
		lm->importedMaterials) {

		materialPointers.push_back(
			materialResources
			? &materialResources->materialInstance
			: &lm->materialInstance);
	}

	if (!materialPointers.empty()) {
		mr->setMaterialInstances(
			materialPointers);
	}
	else {
		mr->setMaterialInstance(
			&lm->materialInstance);
	}

	mr->setVisible(true);
	mr->setCastShadow(true);

	m_actorSourcePaths[a.get()] = modelPath;
	m_loadedModels.push_back(std::move(lm));
	return a;
}

bool
BaseApp::getActorAABB(const EU::TSharedPointer<Actor>& actor, EU::Vector3& outMin, EU::Vector3& outMax) {
	if (actor.isNull()) return false;
	EU::TSharedPointer<MeshRendererComponent> mr = actor->getComponent<MeshRendererComponent>();
	if (!mr) return false;
	Mesh* mesh = mr->getMesh();
	if (!mesh) return false;

	if (mesh == &m_carRenderMesh) { outMin = m_carModelLocalMin; outMax = m_carModelLocalMax; return true; }
	for (auto& lm : m_loadedModels) {
		if (lm && &lm->mesh == mesh) { outMin = lm->localMin; outMax = lm->localMax; return true; }
	}
	return false;
}

EU::TSharedPointer<Actor>
BaseApp::getSelectedActor() const {
    if (m_gui.selectedActorIndex < 0 ||
        m_gui.selectedActorIndex >= static_cast<int>(m_actors.size())) {
        return EU::TSharedPointer<Actor>();
    }
    return m_actors[m_gui.selectedActorIndex];
}

EU::TSharedPointer<Actor>
BaseApp::findActorShared(Actor* actor) const {
    if (!actor) return EU::TSharedPointer<Actor>();

    for (const auto& candidate : m_actors) {
        if (!candidate.isNull() && candidate.get() == actor) {
            return candidate;
        }
    }
    return EU::TSharedPointer<Actor>();
}

std::string
BaseApp::makeUniqueActorName(
    const std::string& requestedName,
    const Actor* ignoredActor) const {
    const std::string baseName = requestedName.empty()
        ? "Actor"
        : requestedName;

    auto nameExists = [&](const std::string& candidateName) {
        for (const auto& actor : m_actors) {
            if (actor.isNull() || actor.get() == ignoredActor) continue;
            if (actor->getName() == candidateName) return true;
        }
        return false;
    };

    if (!nameExists(baseName)) return baseName;

    for (unsigned int suffix = 2; suffix < 100000; ++suffix) {
        const std::string candidate =
            baseName + " (" + std::to_string(suffix) + ")";
        if (!nameExists(candidate)) return candidate;
    }

    return baseName + "_unique";
}

EU::TSharedPointer<Actor>
BaseApp::cloneActor(
    const EU::TSharedPointer<Actor>& source,
    const std::string& newName,
    const EU::Vector3& newPosition) {
    if (source.isNull()) return EU::TSharedPointer<Actor>();

    EU::TSharedPointer<Transform> sourceTransform =
        source->getComponent<Transform>();
    if (!sourceTransform) return EU::TSharedPointer<Actor>();

    EU::TSharedPointer<Actor> clone;
    EU::TSharedPointer<LightComponent> sourceLight =
        source->getComponent<LightComponent>();
    EU::TSharedPointer<MeshRendererComponent> sourceRenderer =
        source->getComponent<MeshRendererComponent>();

    if (sourceLight) {
        const LightData& data = sourceLight->getLightData();
        clone = spawnLightActor(
            data.type,
            newName,
            newPosition,
            data.color,
            data.intensity,
            data.range,
            data.castShadow);

        if (!clone.isNull()) {
            EU::TSharedPointer<LightComponent> clonedLight =
                clone->getComponent<LightComponent>();
            if (clonedLight) {
                clonedLight->setEnabled(data.enabled);
                clonedLight->setDirection(data.direction);
                clonedLight->setSpotAngles(
                    data.innerSpotAngle,
                    data.spotAngle);
                clonedLight->setCastShadow(data.castShadow);
                clonedLight->setFollowTransformPosition(
                    sourceLight->followsTransformPosition());
                clonedLight->setFollowTransformDirection(
                    sourceLight->followsTransformDirection());
            }
        }
    }
    else {
        clone = EU::MakeShared<Actor>(m_device);
        if (!clone.isNull()) {
            clone->setName(newName);

            if (sourceRenderer && sourceRenderer->hasMesh()) {
                EU::TSharedPointer<MeshRendererComponent> clonedRenderer =
                    clone->getComponent<MeshRendererComponent>();
                if (!clonedRenderer) {
                    clonedRenderer = EU::MakeShared<MeshRendererComponent>();
                    clone->addComponent(clonedRenderer);
                }

                clonedRenderer->setMesh(sourceRenderer->getMesh());
                clonedRenderer->setMaterialInstances(
                    sourceRenderer->getMaterialInstances());
                clonedRenderer->setVisible(sourceRenderer->isVisible());
                clonedRenderer->setCastShadow(
                    sourceRenderer->canCastShadow());
                clonedRenderer->setReceiveShadow(
                    sourceRenderer->canReceiveShadow());
                clonedRenderer->setSelectable(
                    sourceRenderer->isSelectable());
            }

            const auto sourcePathIterator =
                m_actorSourcePaths.find(source.get());
            if (sourcePathIterator != m_actorSourcePaths.end()) {
                m_actorSourcePaths[clone.get()] = sourcePathIterator->second;
            }
        }
    }

    if (clone.isNull()) return clone;

    clone->setName(newName);
    clone->setActive(source->isActive());
    clone->setCastShadow(source->canCastShadow());

    EU::TSharedPointer<Transform> clonedTransform =
        clone->getComponent<Transform>();
    if (clonedTransform) {
        clonedTransform->setTransform(
            newPosition,
            sourceTransform->getRotation(),
            sourceTransform->getScale());
    }

    return clone;
}


EU::TSharedPointer<Actor>
BaseApp::spawnLightActor(
    LightType type,
    const std::string& name,
    const EU::Vector3& position,
    const EU::Vector3& color,
    float intensity,
    float range,
    bool castShadow) {
    EU::TSharedPointer<Actor> actor = EU::MakeShared<Actor>(m_device);
    if (actor.isNull()) {
        ERROR("BaseApp", "spawnLightActor", "No se pudo crear el actor de luz");
        return actor;
    }

    actor->setName(name);

    EU::TSharedPointer<Transform> transform = actor->getComponent<Transform>();
    if (transform) {
        transform->setPosition(position);
    }

    EU::TSharedPointer<LightComponent> lightComponent =
        actor->getComponent<LightComponent>();
    if (!lightComponent) {
        lightComponent = EU::MakeShared<LightComponent>(type);
        actor->addComponent(lightComponent);
    }

    lightComponent->setType(type);
    lightComponent->setColor(color);
    lightComponent->setIntensity(intensity);
    lightComponent->setRange(range);
    lightComponent->setCastShadow(castShadow);
    lightComponent->setEnabled(true);
    lightComponent->setFollowTransformPosition(type != LightType::Directional);
    lightComponent->setFollowTransformDirection(false);

    if (type == LightType::Spot) {
        lightComponent->setSpotAngles(20.0f, 35.0f);
    }

    return actor;
}

void
BaseApp::aimLightAtActor(
    const EU::TSharedPointer<Actor>& lightActor,
    const EU::TSharedPointer<Actor>& targetActor) {
    if (lightActor.isNull() || targetActor.isNull()) {
        return;
    }

    EU::TSharedPointer<LightComponent> lightComponent =
        lightActor->getComponent<LightComponent>();
    EU::TSharedPointer<Transform> lightTransform =
        lightActor->getComponent<Transform>();
    EU::TSharedPointer<Transform> targetTransform =
        targetActor->getComponent<Transform>();

    if (!lightComponent || !lightTransform || !targetTransform) {
        return;
    }

    EU::Vector3 localMinimum;
    EU::Vector3 localMaximum;
    if (!getActorAABB(targetActor, localMinimum, localMaximum)) {
        return;
    }

    const XMVECTOR localCenter = XMVectorSet(
        (localMinimum.x + localMaximum.x) * 0.5f,
        (localMinimum.y + localMaximum.y) * 0.5f,
        (localMinimum.z + localMaximum.z) * 0.5f,
        1.0f);
    const XMVECTOR targetWorldCenter = XMVector3TransformCoord(
        localCenter,
        targetTransform->worldMatrix);

    XMFLOAT3 targetPosition{};
    XMStoreFloat3(&targetPosition, targetWorldCenter);

    const EU::Vector3 direction =
        EU::Vector3(targetPosition.x, targetPosition.y, targetPosition.z) -
        lightTransform->getPosition();

    if (!direction.isNearlyZero()) {
        lightComponent->setDirection(direction);
        lightComponent->setFollowTransformDirection(false);
        MESSAGE("BaseApp", "aimLightAtActor", "Luz apuntada al modelo seleccionado");
    }
}

void
BaseApp::createStudioLightRig() {
    EU::TSharedPointer<Actor> target = m_lastSelectedRenderableActor;
    if (target.isNull()) {
        target = getSelectedActor();
    }

    if (target.isNull() ||
        !target->getComponent<MeshRendererComponent>()) {
        ERROR("BaseApp", "createStudioLightRig",
            "Selecciona primero un modelo para crear el rig de estudio");
        return;
    }

    EU::Vector3 localMinimum;
    EU::Vector3 localMaximum;
    EU::TSharedPointer<Transform> targetTransform =
        target->getComponent<Transform>();
    if (!targetTransform ||
        !getActorAABB(target, localMinimum, localMaximum)) {
        ERROR("BaseApp", "createStudioLightRig",
            "No se pudo calcular el tamano del modelo seleccionado");
        return;
    }

    const XMVECTOR localCenter = XMVectorSet(
        (localMinimum.x + localMaximum.x) * 0.5f,
        (localMinimum.y + localMaximum.y) * 0.5f,
        (localMinimum.z + localMaximum.z) * 0.5f,
        1.0f);
    const XMVECTOR worldCenterVector = XMVector3TransformCoord(
        localCenter,
        targetTransform->worldMatrix);

    XMFLOAT3 worldCenterValues{};
    XMStoreFloat3(&worldCenterValues, worldCenterVector);
    const EU::Vector3 center(
        worldCenterValues.x,
        worldCenterValues.y,
        worldCenterValues.z);

    const EU::Vector3 extents(
        (localMaximum.x - localMinimum.x) * 0.5f,
        (localMaximum.y - localMinimum.y) * 0.5f,
        (localMaximum.z - localMinimum.z) * 0.5f);
    const EU::Vector3 targetScale = targetTransform->getScale();
    float radius = (std::max)(
        extents.x * EU::abs(targetScale.x),
        (std::max)(
            extents.y * EU::abs(targetScale.y),
            extents.z * EU::abs(targetScale.z)));
    radius = (std::max)(radius, 1.0f);

    struct StudioLightDescription {
        const char* name;
        EU::Vector3 offset;
        EU::Vector3 color;
        float intensity;
        bool castShadow;
    };

    const StudioLightDescription descriptions[3] = {
        {
            "Studio Key Light",
            EU::Vector3(-1.6f, 1.8f, -1.5f),
            EU::Vector3(1.0f, 0.86f, 0.72f),
            4.5f,
            true
        },
        {
            "Studio Fill Light",
            EU::Vector3(1.7f, 0.9f, -1.1f),
            EU::Vector3(0.72f, 0.86f, 1.0f),
            2.2f,
            false
        },
        {
            "Studio Rim Light",
            EU::Vector3(0.2f, 1.5f, 1.9f),
            EU::Vector3(0.78f, 0.92f, 1.0f),
            3.2f,
            false
        }
    };

    int firstCreatedIndex = -1;
    for (const StudioLightDescription& description : descriptions) {
        const EU::Vector3 lightPosition = center +
            description.offset * radius;
        EU::TSharedPointer<Actor> lightActor = spawnLightActor(
            LightType::Spot,
            description.name,
            lightPosition,
            description.color,
            description.intensity,
            radius * 6.0f,
            description.castShadow);

        if (lightActor) {
            addActorToScene(lightActor);
            if (firstCreatedIndex < 0) {
                firstCreatedIndex = static_cast<int>(m_actors.size()) - 1;
            }
            aimLightAtActor(lightActor, target);
        }
    }

    if (firstCreatedIndex >= 0) {
        m_gui.selectedActorIndex = firstCreatedIndex;
        MESSAGE("BaseApp", "createStudioLightRig",
            "Rig Key, Fill y Rim creado alrededor del modelo");
    }
}
