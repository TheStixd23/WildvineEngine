#pragma once
#include "Prerequisites.h"
#include "ECS/Component.h"

class Mesh;
class MaterialInstance;
class DeviceContext;

class MeshRendererComponent : public Component {
public:
    MeshRendererComponent()
        : Component(ComponentType::MESH) {}

    void init() override {}
    void update(float deltaTime) override { (void)deltaTime; }
    void render(DeviceContext& deviceContext) override { (void)deviceContext; }

    void destroy() override {
        m_mesh = nullptr;
        m_materialInstance = nullptr;
        m_materialInstances.clear();
    }

    void setMesh(Mesh* mesh) { m_mesh = mesh; }
    Mesh* getMesh() const { return m_mesh; }
    bool hasMesh() const { return m_mesh != nullptr; }

    void setMaterialInstance(MaterialInstance* materialInstance) {
        m_materialInstance = materialInstance;
        m_materialInstances.clear();
        if (materialInstance) {
            m_materialInstances.push_back(materialInstance);
        }
    }

    MaterialInstance* getMaterialInstance() const {
        return m_materialInstance;
    }

    MaterialInstance* getMaterialInstance(size_t materialSlot) const {
        if (materialSlot < m_materialInstances.size()) {
            return m_materialInstances[materialSlot];
        }
        return m_materialInstance;
    }

    void setMaterialInstances(const std::vector<MaterialInstance*>& materialInstances) {
        // Se conservan los huecos porque materialSlot usa el indice exacto.
        m_materialInstances = materialInstances;
        m_materialInstance = nullptr;
        for (MaterialInstance* materialInstance : m_materialInstances) {
            if (materialInstance) {
                m_materialInstance = materialInstance;
                break;
            }
        }
    }

    void addMaterialInstance(MaterialInstance* materialInstance) {
        if (!materialInstance) {
            return;
        }

        if (std::find(m_materialInstances.begin(),
                m_materialInstances.end(),
                materialInstance) != m_materialInstances.end()) {
            return;
        }

        if (!m_materialInstance) {
            m_materialInstance = materialInstance;
        }
        m_materialInstances.push_back(materialInstance);
    }

    void clearMaterialInstances() {
        m_materialInstance = nullptr;
        m_materialInstances.clear();
    }

    const std::vector<MaterialInstance*>& getMaterialInstances() const {
        return m_materialInstances;
    }

    size_t getMaterialCount() const {
        return m_materialInstances.size();
    }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    bool canCastShadow() const { return m_castShadow; }
    void setCastShadow(bool value) { m_castShadow = value; }

    bool canReceiveShadow() const { return m_receiveShadow; }
    void setReceiveShadow(bool value) { m_receiveShadow = value; }

    bool isSelectable() const { return m_selectable; }
    void setSelectable(bool value) { m_selectable = value; }

private:
    Mesh* m_mesh = nullptr;
    MaterialInstance* m_materialInstance = nullptr;
    std::vector<MaterialInstance*> m_materialInstances;
    bool m_visible = true;
    bool m_castShadow = true;
    bool m_receiveShadow = true;
    bool m_selectable = true;
};
