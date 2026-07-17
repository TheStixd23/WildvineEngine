#include "ECS/Actor.h"
#include "ECS/MeshRendererComponent.h"
#include "MeshComponent.h"
#include "Device.h"
#include "DeviceContext.h"

int Actor::s_nextActorId = 1;

Actor::Actor() {
    initializeIdentity();
}

Actor::Actor(Device& device) {
    initializeIdentity();

    EU::TSharedPointer<Transform> transform = EU::MakeShared<Transform>();
    addComponent(transform);

    EU::TSharedPointer<MeshComponent> meshComponent = EU::MakeShared<MeshComponent>();
    addComponent(meshComponent);

    const std::string classNameType = "Actor -> " + m_name;
    HRESULT hr = m_modelBuffer.init(device, sizeof(CBChangesEveryFrame));
    if (FAILED(hr)) {
        ERROR("Actor", classNameType.c_str(), "Failed to create CBChangesEveryFrame");
    }

    awake();

    hr = m_sampler.init(device);
    if (FAILED(hr)) {
        ERROR("Actor", classNameType.c_str(), "Failed to create SamplerState");
    }
}

void Actor::initializeIdentity() {
    m_id = s_nextActorId++;
    m_isActive = true;
    m_destroyed = false;
    castShadow = true;
}

void Actor::update(float deltaTime, DeviceContext& deviceContext) {
    if (!m_isActive || m_destroyed) {
        return;
    }

    for (auto& component : m_components) {
        if (component) {
            component->update(deltaTime);
        }
    }

    EU::TSharedPointer<Transform> transform = getComponent<Transform>();
    if (!transform) {
        return;
    }

    m_model.mWorld = XMMatrixTranspose(transform->matrix);
    m_model.vMeshColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    m_modelBuffer.update(deviceContext, nullptr, 0, nullptr, &m_model, 0, 0);
}

void Actor::render(DeviceContext& deviceContext) {
    if (!m_isActive || m_destroyed) {
        return;
    }

    m_sampler.render(deviceContext, 0, 1);
    deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const size_t renderableMeshCount = (std::min)(
        m_meshes.size(),
        (std::min)(m_vertexBuffers.size(), m_indexBuffers.size()));

    for (size_t meshIndex = 0; meshIndex < renderableMeshCount; ++meshIndex) {
        m_vertexBuffers[meshIndex].render(deviceContext, 0, 1);
        m_indexBuffers[meshIndex].render(
            deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
        m_modelBuffer.render(deviceContext, 1, 1, true);

        ID3D11ShaderResourceView* nullResources[8] = {};
        deviceContext.m_deviceContext->PSSetShaderResources(0, 8, nullResources);

        for (size_t textureIndex = 0;
            textureIndex < m_textures.size() && textureIndex < 8;
            ++textureIndex) {
            m_textures[textureIndex].render(
                deviceContext,
                static_cast<unsigned int>(textureIndex),
                1);
        }

        deviceContext.DrawIndexed(m_meshes[meshIndex].m_numIndex, 0, 0);
    }
}

void Actor::renderForSkybox(DeviceContext& deviceContext) {
    if (!m_isActive || m_destroyed) {
        return;
    }

    deviceContext.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const size_t renderableMeshCount = (std::min)(
        m_meshes.size(),
        (std::min)(m_vertexBuffers.size(), m_indexBuffers.size()));

    for (size_t meshIndex = 0; meshIndex < renderableMeshCount; ++meshIndex) {
        m_vertexBuffers[meshIndex].render(deviceContext, 0, 1);
        m_indexBuffers[meshIndex].render(
            deviceContext, 0, 1, false, DXGI_FORMAT_R32_UINT);
        deviceContext.DrawIndexed(m_meshes[meshIndex].m_numIndex, 0, 0);
    }
}

void Actor::destroy() {
    if (m_destroyed) {
        return;
    }

    for (auto& component : m_components) {
        if (component) {
            component->destroy();
        }
    }

    for (auto& vertexBuffer : m_vertexBuffers) {
        vertexBuffer.destroy();
    }
    for (auto& indexBuffer : m_indexBuffers) {
        indexBuffer.destroy();
    }
    for (auto& texture : m_textures) {
        texture.destroy();
    }

    m_vertexBuffers.clear();
    m_indexBuffers.clear();
    m_meshes.clear();
    m_textures.clear();

    m_modelBuffer.destroy();
    m_sampler.destroy();

    m_isActive = false;
    m_destroyed = true;
}

void Actor::setMesh(Device& device, std::vector<MeshComponent> meshes) {
    for (auto& vertexBuffer : m_vertexBuffers) {
        vertexBuffer.destroy();
    }
    for (auto& indexBuffer : m_indexBuffers) {
        indexBuffer.destroy();
    }

    m_vertexBuffers.clear();
    m_indexBuffers.clear();
    m_meshes.clear();

    m_vertexBuffers.reserve(meshes.size());
    m_indexBuffers.reserve(meshes.size());
    m_meshes.reserve(meshes.size());

    for (auto& mesh : meshes) {
        Buffer vertexBuffer;
        HRESULT hr = vertexBuffer.init(device, mesh, D3D11_BIND_VERTEX_BUFFER);
        if (FAILED(hr)) {
            ERROR("Actor", "setMesh", "Failed to create vertex buffer");
            continue;
        }

        Buffer indexBuffer;
        hr = indexBuffer.init(device, mesh, D3D11_BIND_INDEX_BUFFER);
        if (FAILED(hr)) {
            vertexBuffer.destroy();
            ERROR("Actor", "setMesh", "Failed to create index buffer");
            continue;
        }

        m_meshes.push_back(std::move(mesh));
        m_vertexBuffers.push_back(vertexBuffer);
        m_indexBuffers.push_back(indexBuffer);
    }
}

void Actor::setCastShadow(bool value) {
    castShadow = value;
    EU::TSharedPointer<MeshRendererComponent> meshRenderer =
        getComponent<MeshRendererComponent>();
    if (meshRenderer) {
        meshRenderer->setCastShadow(value);
    }
}

void Actor::renderShadow(DeviceContext& deviceContext) {
    if (!m_isActive || m_destroyed || !castShadow) {
        return;
    }
    render(deviceContext);
}
