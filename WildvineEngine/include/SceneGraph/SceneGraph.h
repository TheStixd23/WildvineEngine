#pragma once
#include "Prerequisites.h"

class Entity;
class DeviceContext;
class Camera;
class RenderScene;

/**
 * @class SceneGraph
 * @brief Gestiona registro, jerarquia y propagacion de transformaciones.
 *
 * SceneGraph no es propietario de las entidades. BaseApp conserva la vida de
 * los Actor mediante TSharedPointer y SceneGraph mantiene referencias seguras
 * mientras esos actores formen parte de la escena.
 */
class SceneGraph {
public:
    SceneGraph() = default;
    ~SceneGraph() = default;

    void init();
    void destroy();

    void addEntity(Entity* entity);

    /**
     * @brief Quita una entidad del grafo.
     * @param entity Entidad que se eliminara del registro.
     * @param keepChildrenWorldTransform Mantiene visualmente a sus hijos en el
     * mismo lugar al convertirlos en nodos raiz.
     */
    void removeEntity(
        Entity* entity,
        bool keepChildrenWorldTransform = true);

    bool isAncestor(Entity* possibleAncestor, Entity* node) const;

    /**
     * @brief Coloca child debajo de parent.
     * @param keepWorldTransform Si es true, conserva la transformacion global.
     */
    bool attach(
        Entity* child,
        Entity* parent,
        bool keepWorldTransform = true);

    /**
     * @brief Convierte child en nodo raiz.
     * @param keepWorldTransform Si es true, conserva la transformacion global.
     */
    bool detach(
        Entity* child,
        bool keepWorldTransform = true);

    /** @brief Alias explicito de attach/detach para la interfaz del editor. */
    bool reparent(
        Entity* child,
        Entity* newParent,
        bool keepWorldTransform = true);

    Entity* getParent(Entity* entity) const;
    const std::vector<Entity*>* getChildren(Entity* entity) const;
    std::vector<Entity*> getRootEntities() const;

    Entity* findEntityById(int id) const;
    Entity* findEntityByName(const std::string& name) const;

    bool isRegistered(Entity* entity) const;
    bool isRoot(Entity* entity) const;

    size_t getEntityCount() const { return m_entities.size(); }
    const std::vector<Entity*>& getEntities() const { return m_entities; }

    /**
     * @brief Comprueba referencias rotas, hijos duplicados y ciclos.
     * @param repair Si es true intenta reparar automaticamente los problemas.
     * @return true si la jerarquia quedo valida.
     */
    bool validateHierarchy(bool repair = false);

    void update(float deltaTime, DeviceContext& deviceContext);
    void render(DeviceContext& deviceContext);
    void gatherRenderScene(RenderScene& outScene, const Camera& camera);

private:
    void ensureRequiredComponents(Entity* entity);

    void updateWorldRecursive(
        Entity* node,
        const XMMATRIX& parentWorld,
        std::vector<Entity*>& recursionPath);

    XMMATRIX calculateWorldMatrix(Entity* entity) const;
    bool applyLocalMatrix(Entity* entity, const XMMATRIX& localMatrix);

    bool hasEntityPointer(const std::vector<Entity*>& entities, Entity* value) const;
    void removeAllChildReferences(Entity* child);

private:
    std::vector<Entity*> m_entities;
};
