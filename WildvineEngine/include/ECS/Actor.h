#pragma once
#include "Prerequisites.h"
#include "Entity.h"
#include "Buffer.h"
#include "Texture.h"
#include "Transform.h"
#include "SamplerState.h"
#include "RasterizerState.h"
#include "ShaderProgram.h"
#include "DepthStencilState.h"

class Device;
class DeviceContext;
class MeshComponent;

class Actor : public Entity {
public:
    Actor();
    explicit Actor(Device& device);
    virtual ~Actor() = default;

    void awake() override {}
    void init() override {}
    void update(float deltaTime, DeviceContext& deviceContext) override;
    void render(DeviceContext& deviceContext) override;
    void renderForSkybox(DeviceContext& deviceContext);
    void destroy() override;

    void setMesh(Device& device, std::vector<MeshComponent> meshes);

    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) {
        m_name = name.empty() ? "Actor" : name;
    }

    int getId() const { return m_id; }

    bool isActive() const { return m_isActive; }
    void setActive(bool active) { m_isActive = active; }

    void setTextures(const std::vector<Texture>& textures) {
        m_textures = textures;
    }

    void setCastShadow(bool value);
    bool canCastShadow() const { return castShadow; }

    bool isDestroyed() const { return m_destroyed; }
    void renderShadow(DeviceContext& deviceContext);

private:
    void initializeIdentity();

private:
    std::vector<MeshComponent> m_meshes;
    std::vector<Texture> m_textures;
    std::vector<Buffer> m_vertexBuffers;
    std::vector<Buffer> m_indexBuffers;

    SamplerState m_sampler;
    CBChangesEveryFrame m_model{};
    Buffer m_modelBuffer;

    ShaderProgram m_shaderShadow;
    Buffer m_shaderBuffer;
    DepthStencilState m_shadowDepthStencilState;
    CBChangesEveryFrame m_cbShadow{};

    XMFLOAT4 m_LightPos = XMFLOAT4(0.0f, 3.0f, 0.0f, 1.0f);
    std::string m_name = "Actor";
    bool castShadow = true;
    bool m_destroyed = false;

    static int s_nextActorId;
};
