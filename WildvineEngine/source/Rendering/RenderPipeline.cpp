/**
 * @file RenderPipeline.cpp
 * @brief Implementa el selector de renderer (Forward/Deferred).
 */
#include "Rendering/RenderPipeline.h"

HRESULT
RenderPipeline::init(Device& device, RendererType type) {
	m_type = type;
	m_active = (type == RendererType::Forward)
		? static_cast<ISceneRenderer*>(&m_forwardRenderer)
		: static_cast<ISceneRenderer*>(&m_deferredRenderer);
	return m_active->init(device);
}

void
RenderPipeline::resize(Device& device, unsigned int width, unsigned int height) {
	if (m_active) m_active->resize(device, width, height);
}

void
RenderPipeline::render(DeviceContext& deviceContext,
	const Camera& camera,
	RenderScene& scene,
	EditorViewportPass& viewportPass) {
	if (m_active) m_active->render(deviceContext, camera, scene, viewportPass);
}

void
RenderPipeline::destroy() {
	if (m_active) {
		m_active->destroy();
		m_active = nullptr;
	}
}
