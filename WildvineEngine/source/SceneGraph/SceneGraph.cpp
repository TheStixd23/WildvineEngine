/**
 * @file SceneGraph.cpp
 * @brief Gestion segura de jerarquia, transformaciones y recopilacion de render.
 */
#include "SceneGraph\SceneGraph.h"
#include "SceneGraph\HierarchyComponent.h"
#include "ECS\Entity.h"
#include "ECS\Actor.h"
#include "ECS\Transform.h"
#include "ECS\LightComponent.h"
#include "ECS\MeshRendererComponent.h"
#include "ECS/ParticleEmitterComponent.h"
#include "DeviceContext.h"
#include "EngineUtilities/Utilities/Camera.h"
#include "Rendering/Material.h"
#include "Rendering/MaterialInstance.h"
#include "Rendering/Mesh.h"
#include "Rendering/RenderScene.h"
#include "Rendering/Frustum.h"
#include "Rendering/PerformanceProfiler.h"
#include "Rendering/Octree.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

void SceneGraph::init() {
    m_entities.clear();
}

void SceneGraph::destroy() {
    for (Entity* entity : m_entities) {
        if (!entity) continue;

        EU::TSharedPointer<HierarchyComponent> hierarchy =
            entity->getComponent<HierarchyComponent>();
        if (!hierarchy) continue;

        hierarchy->m_parent = nullptr;
        hierarchy->m_children.clear();
    }

    m_entities.clear();
}

void SceneGraph::ensureRequiredComponents(Entity* entity) {
    if (!entity) return;

    EU::TSharedPointer<Transform> transform =
        entity->getComponent<Transform>();
    if (!transform) {
        transform = EU::MakeShared<Transform>();
        entity->addComponent(transform);
        transform->init();
    }

    EU::TSharedPointer<HierarchyComponent> hierarchy =
        entity->getComponent<HierarchyComponent>();
    if (!hierarchy) {
        hierarchy = EU::MakeShared<HierarchyComponent>();
        entity->addComponent(hierarchy);
        hierarchy->init();
    }
}

void SceneGraph::addEntity(Entity* entity) {
    if (!entity || isRegistered(entity)) return;

    ensureRequiredComponents(entity);
    m_entities.push_back(entity);
}

bool SceneGraph::hasEntityPointer(
    const std::vector<Entity*>& entities,
    Entity* value) const {
    return std::find(entities.begin(), entities.end(), value) != entities.end();
}

void SceneGraph::removeAllChildReferences(Entity* child) {
    if (!child) return;

    for (Entity* entity : m_entities) {
        if (!entity || entity == child) continue;

        EU::TSharedPointer<HierarchyComponent> hierarchy =
            entity->getComponent<HierarchyComponent>();
        if (hierarchy) {
            hierarchy->removeChild(child);
        }
    }
}

void SceneGraph::removeEntity(
    Entity* entity,
    bool keepChildrenWorldTransform) {
    if (!entity || !isRegistered(entity)) return;

    EU::TSharedPointer<HierarchyComponent> hierarchy =
        entity->getComponent<HierarchyComponent>();

    if (hierarchy) {
        const std::vector<Entity*> childrenCopy = hierarchy->m_children;
        for (Entity* child : childrenCopy) {
            if (!child) continue;

            EU::TSharedPointer<HierarchyComponent> childHierarchy =
                child->getComponent<HierarchyComponent>();
            if (childHierarchy && childHierarchy->m_parent == entity) {
                detach(child, keepChildrenWorldTransform);
            }
        }
    }

    detach(entity, false);
    removeAllChildReferences(entity);

    if (hierarchy) {
        hierarchy->m_parent = nullptr;
        hierarchy->m_children.clear();
    }

    m_entities.erase(
        std::remove(m_entities.begin(), m_entities.end(), entity),
        m_entities.end());
}

bool SceneGraph::isAncestor(
    Entity* possibleAncestor,
    Entity* node) const {
    if (!possibleAncestor || !node) return false;

    Entity* current = node;
    std::vector<Entity*> visited;
    visited.reserve(m_entities.size());

    while (current) {
        if (hasEntityPointer(visited, current)) {
            ERROR("SceneGraph", "isAncestor",
                "Se detecto una jerarquia ciclica o corrupta.");
            return false;
        }
        visited.push_back(current);

        EU::TSharedPointer<HierarchyComponent> hierarchy =
            current->getComponent<HierarchyComponent>();
        if (!hierarchy || !hierarchy->m_parent) return false;

        if (hierarchy->m_parent == possibleAncestor) return true;
        current = hierarchy->m_parent;
    }

    return false;
}

bool SceneGraph::isRoot(Entity* entity) const {
    if (!entity) return false;

    EU::TSharedPointer<HierarchyComponent> hierarchy =
        entity->getComponent<HierarchyComponent>();
    return !hierarchy || hierarchy->m_parent == nullptr;
}

bool SceneGraph::isRegistered(Entity* entity) const {
    return entity &&
        std::find(m_entities.begin(), m_entities.end(), entity) !=
        m_entities.end();
}

Entity* SceneGraph::getParent(Entity* entity) const {
    if (!entity) return nullptr;

    EU::TSharedPointer<HierarchyComponent> hierarchy =
        entity->getComponent<HierarchyComponent>();
    return hierarchy ? hierarchy->m_parent : nullptr;
}

const std::vector<Entity*>* SceneGraph::getChildren(Entity* entity) const {
    if (!entity) return nullptr;

    EU::TSharedPointer<HierarchyComponent> hierarchy =
        entity->getComponent<HierarchyComponent>();
    return hierarchy ? &hierarchy->m_children : nullptr;
}

std::vector<Entity*> SceneGraph::getRootEntities() const {
    std::vector<Entity*> roots;
    roots.reserve(m_entities.size());

    for (Entity* entity : m_entities) {
        if (entity && isRoot(entity)) {
            roots.push_back(entity);
        }
    }
    return roots;
}

Entity* SceneGraph::findEntityById(int id) const {
    for (Entity* entity : m_entities) {
        Actor* actor = dynamic_cast<Actor*>(entity);
        if (actor && actor->getId() == id) return entity;
    }
    return nullptr;
}

Entity* SceneGraph::findEntityByName(const std::string& name) const {
    for (Entity* entity : m_entities) {
        Actor* actor = dynamic_cast<Actor*>(entity);
        if (actor && actor->getName() == name) return entity;
    }
    return nullptr;
}

XMMATRIX SceneGraph::calculateWorldMatrix(Entity* entity) const {
    if (!entity) return XMMatrixIdentity();

    EU::TSharedPointer<Transform> transform =
        entity->getComponent<Transform>();
    if (!transform) return XMMatrixIdentity();

    transform->rebuildLocalMatrix();
    XMMATRIX world = transform->matrix;

    std::vector<Entity*> visited;
    visited.reserve(m_entities.size());
    visited.push_back(entity);

    Entity* current = entity;
    while (current) {
        EU::TSharedPointer<HierarchyComponent> hierarchy =
            current->getComponent<HierarchyComponent>();
        Entity* parent = hierarchy ? hierarchy->m_parent : nullptr;
        if (!parent) break;

        if (hasEntityPointer(visited, parent)) {
            ERROR("SceneGraph", "calculateWorldMatrix",
                "No se pudo calcular la matriz por un ciclo de jerarquia.");
            break;
        }
        visited.push_back(parent);

        EU::TSharedPointer<Transform> parentTransform =
            parent->getComponent<Transform>();
        if (parentTransform) {
            parentTransform->rebuildLocalMatrix();
            world = world * parentTransform->matrix;
        }

        current = parent;
    }

    return world;
}

bool SceneGraph::applyLocalMatrix(
    Entity* entity,
    const XMMATRIX& localMatrix) {
    if (!entity) return false;

    EU::TSharedPointer<Transform> transform =
        entity->getComponent<Transform>();
    if (!transform) return false;

    return transform->setFromLocalMatrix(localMatrix);
}

bool SceneGraph::attach(
    Entity* child,
    Entity* parent,
    bool keepWorldTransform) {
    if (!child || !parent || child == parent) return false;

    addEntity(child);
    addEntity(parent);

    if (isAncestor(child, parent)) {
        ERROR("SceneGraph", "attach",
            "No se puede crear una jerarquia circular.");
        return false;
    }

    EU::TSharedPointer<HierarchyComponent> childHierarchy =
        child->getComponent<HierarchyComponent>();
    EU::TSharedPointer<HierarchyComponent> parentHierarchy =
        parent->getComponent<HierarchyComponent>();
    if (!childHierarchy || !parentHierarchy) return false;

    if (childHierarchy->m_parent == parent) {
        parentHierarchy->addChild(child);
        return true;
    }

    const XMMATRIX childWorld = keepWorldTransform
        ? calculateWorldMatrix(child)
        : XMMatrixIdentity();

    XMMATRIX inverseParentWorld = XMMatrixIdentity();
    if (keepWorldTransform) {
        const XMMATRIX parentWorld = calculateWorldMatrix(parent);
        XMVECTOR determinant = XMVectorZero();
        inverseParentWorld = XMMatrixInverse(&determinant, parentWorld);

        if (fabsf(XMVectorGetX(determinant)) <= 0.000001f) {
            ERROR("SceneGraph", "attach",
                "La transformacion del padre no es invertible.");
            return false;
        }
    }

    if (keepWorldTransform) {
        const XMMATRIX localMatrix = childWorld * inverseParentWorld;
        if (!applyLocalMatrix(child, localMatrix)) {
            ERROR("SceneGraph", "attach",
                "No se pudo conservar la transformacion del hijo.");
            return false;
        }
    }

    detach(child, false);

    childHierarchy->m_parent = parent;
    parentHierarchy->addChild(child);
    return true;
}

bool SceneGraph::detach(
    Entity* child,
    bool keepWorldTransform) {
    if (!child) return false;

    EU::TSharedPointer<HierarchyComponent> childHierarchy =
        child->getComponent<HierarchyComponent>();
    if (!childHierarchy) return false;

    Entity* parent = childHierarchy->m_parent;
    if (!parent) return true;

    const XMMATRIX childWorld = keepWorldTransform
        ? calculateWorldMatrix(child)
        : XMMatrixIdentity();

    EU::TSharedPointer<HierarchyComponent> parentHierarchy =
        parent->getComponent<HierarchyComponent>();
    if (parentHierarchy) parentHierarchy->removeChild(child);

    childHierarchy->m_parent = nullptr;

    if (keepWorldTransform && !applyLocalMatrix(child, childWorld)) {
        ERROR("SceneGraph", "detach",
            "No se pudo conservar la transformacion global.");
        return false;
    }

    return true;
}

bool SceneGraph::reparent(
    Entity* child,
    Entity* newParent,
    bool keepWorldTransform) {
    return newParent
        ? attach(child, newParent, keepWorldTransform)
        : detach(child, keepWorldTransform);
}

bool SceneGraph::validateHierarchy(bool repair) {
    bool valid = true;

    if (repair) {
        std::vector<Entity*> cleanEntities;
        cleanEntities.reserve(m_entities.size());
        for (Entity* entity : m_entities) {
            if (!entity || hasEntityPointer(cleanEntities, entity)) {
                valid = false;
                continue;
            }
            cleanEntities.push_back(entity);
        }
        m_entities.swap(cleanEntities);
    }
    else {
        std::vector<Entity*> seen;
        for (Entity* entity : m_entities) {
            if (!entity || hasEntityPointer(seen, entity)) valid = false;
            if (entity) seen.push_back(entity);
        }
    }

    for (Entity* entity : m_entities) {
        if (!entity) continue;
        ensureRequiredComponents(entity);

        EU::TSharedPointer<HierarchyComponent> hierarchy =
            entity->getComponent<HierarchyComponent>();
        if (!hierarchy) {
            valid = false;
            continue;
        }

        if (hierarchy->m_parent == entity ||
            (hierarchy->m_parent && !isRegistered(hierarchy->m_parent))) {
            valid = false;
            if (repair) hierarchy->m_parent = nullptr;
        }

        std::vector<Entity*> cleanChildren;
        cleanChildren.reserve(hierarchy->m_children.size());
        for (Entity* child : hierarchy->m_children) {
            const bool childValid = child && child != entity &&
                isRegistered(child) && !hasEntityPointer(cleanChildren, child);
            if (!childValid) {
                valid = false;
                continue;
            }

            EU::TSharedPointer<HierarchyComponent> childHierarchy =
                child->getComponent<HierarchyComponent>();
            if (!childHierarchy || childHierarchy->m_parent != entity) {
                valid = false;
                if (repair && childHierarchy) {
                    childHierarchy->m_parent = entity;
                }
            }
            cleanChildren.push_back(child);
        }

        if (repair) hierarchy->m_children.swap(cleanChildren);
    }

    // Asegura que todo padre contenga una referencia a sus hijos.
    for (Entity* entity : m_entities) {
        if (!entity) continue;

        EU::TSharedPointer<HierarchyComponent> hierarchy =
            entity->getComponent<HierarchyComponent>();
        Entity* parent = hierarchy ? hierarchy->m_parent : nullptr;
        if (!parent) continue;

        EU::TSharedPointer<HierarchyComponent> parentHierarchy =
            parent->getComponent<HierarchyComponent>();
        if (!parentHierarchy ||
            !hasEntityPointer(parentHierarchy->m_children, entity)) {
            valid = false;
            if (repair && parentHierarchy) parentHierarchy->addChild(entity);
        }
    }

    // Detecta ciclos recorriendo cada cadena de padres.
    for (Entity* start : m_entities) {
        std::vector<Entity*> chain;
        Entity* current = start;

        while (current) {
            if (hasEntityPointer(chain, current)) {
                valid = false;

                if (repair) {
                    EU::TSharedPointer<HierarchyComponent> hierarchy =
                        current->getComponent<HierarchyComponent>();
                    Entity* oldParent = hierarchy ? hierarchy->m_parent : nullptr;
                    if (oldParent) {
                        EU::TSharedPointer<HierarchyComponent> oldParentHierarchy =
                            oldParent->getComponent<HierarchyComponent>();
                        if (oldParentHierarchy) {
                            oldParentHierarchy->removeChild(current);
                        }
                    }
                    if (hierarchy) hierarchy->m_parent = nullptr;
                }
                break;
            }

            chain.push_back(current);
            current = getParent(current);
        }
    }

    return valid;
}

void SceneGraph::update(
    float deltaTime,
    DeviceContext& deviceContext) {
    for (Entity* entity : m_entities) {
        if (entity) entity->update(deltaTime, deviceContext);
    }

    std::vector<Entity*> recursionPath;
    recursionPath.reserve(m_entities.size());

    const std::vector<Entity*> roots = getRootEntities();
    for (Entity* root : roots) {
        updateWorldRecursive(root, XMMatrixIdentity(), recursionPath);
    }
}

void SceneGraph::updateWorldRecursive(
    Entity* node,
    const XMMATRIX& parentWorld,
    std::vector<Entity*>& recursionPath) {
    if (!node || hasEntityPointer(recursionPath, node)) {
        if (node) {
            ERROR("SceneGraph", "updateWorldRecursive",
                "Se omitio un ciclo detectado durante la actualizacion.");
        }
        return;
    }

    recursionPath.push_back(node);

    EU::TSharedPointer<Transform> transform =
        node->getComponent<Transform>();
    EU::TSharedPointer<HierarchyComponent> hierarchy =
        node->getComponent<HierarchyComponent>();

    if (transform) {
        transform->rebuildLocalMatrix();
        transform->worldMatrix = transform->matrix * parentWorld;
    }

    if (hierarchy && transform) {
        const std::vector<Entity*> childrenCopy = hierarchy->m_children;
        for (Entity* child : childrenCopy) {
            if (!child || !isRegistered(child)) continue;

            EU::TSharedPointer<HierarchyComponent> childHierarchy =
                child->getComponent<HierarchyComponent>();
            if (!childHierarchy || childHierarchy->m_parent != node) continue;

            updateWorldRecursive(
                child,
                transform->worldMatrix,
                recursionPath);
        }
    }

    recursionPath.pop_back();
}

void SceneGraph::render(DeviceContext& deviceContext) {
    for (Entity* entity : m_entities) {
        if (entity) entity->render(deviceContext);
    }
}

void
SceneGraph::gatherRenderScene(
    RenderScene& outScene,
    const Camera& camera,
    const Frustum* frustum,
    PerformanceProfiler* profiler,
    Octree* octree,
    bool validateOctreeResults) {

    if (profiler) {
        profiler->beginCulling();
    }

    // ---------------------------------------------------------------------
    // Octree opcional
    // ---------------------------------------------------------------------
    // Solo indexamos entidades renderizables con AABB valida. Si por cualquier
    // motivo un actor no puede convertirse a AABB mundial, cae al camino
    // conservador del Frustum normal y nunca desaparece por error.
    std::unordered_set<Entity*> octreeIndexedEntities;
    std::unordered_set<Entity*> octreeVisibleEntities;
    std::unordered_set<Entity*> octreeRefinementEntities;

    auto calculateWorldBounds = [](
        const EU::Vector3& localMinimum,
        const EU::Vector3& localMaximum,
        const XMMATRIX& world,
        OctreeBounds& outBounds) -> bool {

        const float largest =
            (std::numeric_limits<float>::max)();
        EU::Vector3 minimum(largest, largest, largest);
        EU::Vector3 maximum(-largest, -largest, -largest);

        for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
            const float x = (cornerIndex & 1)
                ? localMaximum.x
                : localMinimum.x;
            const float y = (cornerIndex & 2)
                ? localMaximum.y
                : localMinimum.y;
            const float z = (cornerIndex & 4)
                ? localMaximum.z
                : localMinimum.z;

            const XMVECTOR worldCorner = XMVector3TransformCoord(
                XMVectorSet(x, y, z, 1.0f),
                world);

            XMFLOAT3 point{};
            XMStoreFloat3(&point, worldCorner);

            if (!std::isfinite(point.x) ||
                !std::isfinite(point.y) ||
                !std::isfinite(point.z)) {
                return false;
            }

            minimum.x = (std::min)(minimum.x, point.x);
            minimum.y = (std::min)(minimum.y, point.y);
            minimum.z = (std::min)(minimum.z, point.z);
            maximum.x = (std::max)(maximum.x, point.x);
            maximum.y = (std::max)(maximum.y, point.y);
            maximum.z = (std::max)(maximum.z, point.z);
        }

        outBounds.minimum = minimum;
        outBounds.maximum = maximum;
        return outBounds.isValid();
    };

    if (frustum && octree) {
        std::vector<OctreeEntry> octreeEntries;
        octreeEntries.reserve(m_entities.size());

        for (Entity* entity : m_entities) {
            if (!entity) {
                continue;
            }

            Actor* actor = dynamic_cast<Actor*>(entity);
            if (actor && !actor->isActive()) {
                continue;
            }

            EU::TSharedPointer<Transform> transform =
                entity->getComponent<Transform>();
            EU::TSharedPointer<MeshRendererComponent> meshRenderer =
                entity->getComponent<MeshRendererComponent>();

            if (!transform ||
                !meshRenderer ||
                !meshRenderer->isVisible() ||
                !meshRenderer->hasMesh()) {
                continue;
            }

            EU::Vector3 localMinimum;
            EU::Vector3 localMaximum;
            if (!meshRenderer->getLocalBounds(
                    localMinimum,
                    localMaximum)) {
                continue;
            }

            OctreeBounds worldBounds{};
            if (!calculateWorldBounds(
                    localMinimum,
                    localMaximum,
                    transform->worldMatrix,
                    worldBounds)) {
                continue;
            }

            OctreeEntry entry{};
            entry.entity = entity;
            entry.bounds = worldBounds;
            octreeEntries.push_back(entry);
            octreeIndexedEntities.insert(entity);
        }

        octree->rebuild(octreeEntries);

        std::vector<Entity*> visibleFromOctree;
        std::vector<Entity*> refinementFromOctree;
        octree->query(
            *frustum,
            visibleFromOctree,
            &refinementFromOctree);

        octreeVisibleEntities.reserve(visibleFromOctree.size());
        for (Entity* visibleEntity : visibleFromOctree) {
            if (visibleEntity) {
                octreeVisibleEntities.insert(visibleEntity);
            }
        }

        octreeRefinementEntities.reserve(refinementFromOctree.size());
        for (Entity* refinementEntity : refinementFromOctree) {
            if (refinementEntity) {
                octreeRefinementEntities.insert(refinementEntity);
            }
        }

        if (profiler) {
            profiler->setOctreeStatistics(
                octree->getStatistics());
        }
    }

    // ---------------------------------------------------------------------
    // Recopilacion de luces y objetos de render
    // ---------------------------------------------------------------------
    for (Entity* entity : m_entities) {
        if (!entity) {
            continue;
        }

        Actor* actor = dynamic_cast<Actor*>(entity);
        if (actor && !actor->isActive()) {
            continue;
        }

        EU::TSharedPointer<Transform> transform =
            entity->getComponent<Transform>();

        EU::TSharedPointer<LightComponent> lightComponent =
            entity->getComponent<LightComponent>();

        if (lightComponent) {
            if (transform) {
                lightComponent->syncWithTransform(*transform);
            }

            const LightData& light = lightComponent->getLightData();
            outScene.addLight(light);
        }

        // -------------------------------------------------------------
        // Particle Emitter
        // -------------------------------------------------------------
        // Los billboards ya contienen vertices en espacio mundial, por eso el
        // RenderObject de particulas usa world = Identity. El emisor se prueba
        // contra el Frustum mediante sus bounds locales dinamicos.
        EU::TSharedPointer<ParticleEmitterComponent> particleEmitter =
            entity->getComponent<ParticleEmitterComponent>();

        if (particleEmitter && transform && particleEmitter->isEnabled()) {
            bool particleVisible = true;
            bool particleHadBounds = false;
            EU::Vector3 particleBoundsMin;
            EU::Vector3 particleBoundsMax;

            particleHadBounds = particleEmitter->getLocalBounds(
                particleBoundsMin,
                particleBoundsMax);

            if (frustum && particleHadBounds) {
                particleVisible = frustum->isBoxVisible(
                    particleBoundsMin,
                    particleBoundsMax,
                    transform->worldMatrix);
            }

            if (profiler) {
                profiler->recordParticleEmitter(
                    particleVisible,
                    particleEmitter->getActiveParticleCount(),
                    particleEmitter->getCapacity(),
                    particleEmitter->getSpawnedThisFrame(),
                    particleEmitter->getLastSimulationTimeMs(),
                    particleEmitter->getLastBillboardTimeMs());
            }

            if (particleVisible && particleEmitter->isRenderReady()) {
                RenderObject particleObject{};
                particleObject.mesh = particleEmitter->getMesh();
                particleObject.materialInstance =
                    particleEmitter->getMaterialInstance();
                particleObject.world = XMMatrixIdentity();
                particleObject.castShadow = false;
                particleObject.receiveShadow = false;
                particleObject.transparent = true;

                XMFLOAT4X4 emitterWorldMatrix{};
                XMStoreFloat4x4(
                    &emitterWorldMatrix,
                    transform->worldMatrix);
                const EU::Vector3 emitterWorldPosition(
                    emitterWorldMatrix._41,
                    emitterWorldMatrix._42,
                    emitterWorldMatrix._43);
                const EU::Vector3 cameraDelta =
                    emitterWorldPosition - camera.getPosition();
                particleObject.distanceToCamera =
                    cameraDelta.magnitudeSquared();

                outScene.transparentObjects.push_back(particleObject);
            }
        }

        EU::TSharedPointer<MeshRendererComponent> meshRenderer =
            entity->getComponent<MeshRendererComponent>();

        if (!meshRenderer ||
            !transform ||
            !meshRenderer->isVisible() ||
            !meshRenderer->hasMesh()) {
            continue;
        }

        bool visibleToCamera = true;
        bool hadBounds = false;

        if (frustum) {
            EU::Vector3 localMinimum;
            EU::Vector3 localMaximum;

            hadBounds = meshRenderer->getLocalBounds(
                localMinimum,
                localMaximum);

            if (hadBounds) {
                const bool indexedByOctree =
                    octree &&
                    octreeIndexedEntities.find(entity) !=
                        octreeIndexedEntities.end();

                if (indexedByOctree) {
                    visibleToCamera =
                        octreeVisibleEntities.find(entity) !=
                        octreeVisibleEntities.end();

                    // El Octree trabaja con AABB en espacio mundo. Cuando esa
                    // caja solo intersecta el frustum hacemos una segunda prueba
                    // mas precisa con las 8 esquinas del bounds local transformado.
                    // Esto mantiene el mismo resultado que el Frustum directo
                    // sin perder la capacidad de aceptar/descartar regiones enteras.
                    const bool needsRefinement =
                        visibleToCamera &&
                        octreeRefinementEntities.find(entity) !=
                            octreeRefinementEntities.end();

                    if (needsRefinement) {
                        if (profiler) {
                            profiler->recordOctreeRefinement();
                        }
                        visibleToCamera = frustum->isBoxVisible(
                            localMinimum,
                            localMaximum,
                            transform->worldMatrix);
                    }

                    // Modo de validacion para presentacion/debug. Es caro porque
                    // ejecuta tambien el Frustum directo, por eso esta apagado
                    // por defecto y nunca modifica el resultado del renderer.
                    if (validateOctreeResults && profiler) {
                        const bool directVisible = frustum->isBoxVisible(
                            localMinimum,
                            localMaximum,
                            transform->worldMatrix);
                        profiler->recordOctreeValidation(
                            directVisible == visibleToCamera);
                    }
                }
                else {
                    // Fallback conservador para objetos que no pudieron entrar
                    // en el Octree: usamos exactamente el Frustum directo.
                    visibleToCamera = frustum->isBoxVisible(
                        localMinimum,
                        localMaximum,
                        transform->worldMatrix);
                }
            }
        }
        else {
            // Culling desactivado. Si existen bounds los contamos para el
            // profiler, pero todos los objetos permanecen visibles.
            hadBounds = meshRenderer->hasLocalBounds();
        }

        unsigned int submeshCount = 0;
        unsigned long long triangleCount = 0;
        if (meshRenderer->getMesh()) {
            const std::vector<Submesh>& profilerSubmeshes =
                meshRenderer->getMesh()->getSubmeshes();
            submeshCount = static_cast<unsigned int>(
                profilerSubmeshes.size());
            for (const Submesh& submesh : profilerSubmeshes) {
                triangleCount += static_cast<unsigned long long>(
                    submesh.indexCount / 3u);
            }
        }

        if (profiler) {
            profiler->recordRenderable(
                visibleToCamera,
                hadBounds,
                submeshCount,
                triangleCount);
        }

        // Politica conservadora: un objeto sin bounds nunca se descarta.
        if (!visibleToCamera) {
            continue;
        }

        RenderObject renderObject{};
        renderObject.mesh = meshRenderer->getMesh();
        renderObject.materialInstance = meshRenderer->getMaterialInstance();
        renderObject.materialInstances = meshRenderer->getMaterialInstances();
        renderObject.world = transform->worldMatrix;
        renderObject.castShadow = meshRenderer->canCastShadow();
        renderObject.receiveShadow = meshRenderer->canReceiveShadow();

        const EU::Vector3 cameraPosition = camera.getPosition();
        XMFLOAT4X4 worldMatrix{};
        XMStoreFloat4x4(&worldMatrix, transform->worldMatrix);
        const EU::Vector3 objectPosition(
            worldMatrix._41,
            worldMatrix._42,
            worldMatrix._43);
        const EU::Vector3 cameraDelta = objectPosition - cameraPosition;
        renderObject.distanceToCamera = cameraDelta.magnitudeSquared();

        bool hasOpaqueSubmeshes = false;
        bool hasTransparentSubmeshes = false;

        const std::vector<Submesh>& submeshes =
            renderObject.mesh->getSubmeshes();

        for (const Submesh& submesh : submeshes) {
            MaterialInstance* materialInstance =
                meshRenderer->getMaterialInstance(submesh.materialSlot);

            Material* material = materialInstance
                ? materialInstance->getMaterial()
                : nullptr;

            const MaterialDomain domain = material
                ? material->getDomain()
                : MaterialDomain::Opaque;

            if (domain == MaterialDomain::Transparent) {
                hasTransparentSubmeshes = true;
            }
            else {
                hasOpaqueSubmeshes = true;
            }
        }

        if (!hasOpaqueSubmeshes && !hasTransparentSubmeshes) {
            hasOpaqueSubmeshes = true;
        }

        if (hasOpaqueSubmeshes) {
            RenderObject opaqueObject = renderObject;
            opaqueObject.transparent = false;
            outScene.opaqueObjects.push_back(opaqueObject);
        }

        if (hasTransparentSubmeshes) {
            RenderObject transparentObject = renderObject;
            transparentObject.transparent = true;
            outScene.transparentObjects.push_back(transparentObject);
        }
    }

    if (profiler) {
        profiler->endCulling();
    }
}
