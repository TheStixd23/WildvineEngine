/**
 * @file RenderPipeline.h
 * @brief Envuelve los renderers (Forward/Deferred) tras una interfaz comun.
 */
#pragma once
#include "Prerequisites.h"
#include "Rendering/ForwardRenderer.h"
#include "Rendering/DeferredRenderer.h"

class Device;
class DeviceContext;
class Camera;
class RenderScene;
class EditorViewportPass;

enum class RendererType { Forward = 0, Deferred = 1 };

class RenderPipeline {
public:
	HRESULT init(Device& device, RendererType type);
	void resize(Device& device, unsigned int width, unsigned int height);
	void render(DeviceContext& deviceContext, const Camera& camera, RenderScene& scene, EditorViewportPass& viewportPass);
	void destroy();

	RendererType getRendererType() const { return m_type; }

	ID3D11ShaderResourceView* getShadowMapSRV() const { return m_active ? m_active->getShadowMapSRV() : nullptr; }
	ID3D11ShaderResourceView* getPreShadowSRV() const { return m_active ? m_active->getPreShadowSRV() : nullptr; }
	ID3D11ShaderResourceView* getGBufferAlbedoMetallicSRV() const { return m_active ? m_active->getGBufferAlbedoMetallicSRV() : nullptr; }
	ID3D11ShaderResourceView* getGBufferNormalRoughnessSRV() const { return m_active ? m_active->getGBufferNormalRoughnessSRV() : nullptr; }
	ID3D11ShaderResourceView* getGBufferWorldAoSRV() const { return m_active ? m_active->getGBufferWorldAoSRV() : nullptr; }
	ID3D11ShaderResourceView* getGBufferEmissiveAlphaSRV() const { return m_active ? m_active->getGBufferEmissiveAlphaSRV() : nullptr; }
	void setShadowFactorDebugEnabled(bool enabled) { if (m_active) m_active->setShadowFactorDebugEnabled(enabled); }
	void setDeferredDebugViewMode(int mode) { if (m_active) m_active->setDeferredDebugViewMode(mode); }

private:
	ForwardRenderer  m_forwardRenderer;
	DeferredRenderer m_deferredRenderer;
	ISceneRenderer* m_active = nullptr;
	RendererType     m_type = RendererType::Deferred;
};
