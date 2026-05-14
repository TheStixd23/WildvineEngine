/**
 * @file RenderScene.h
 * @brief Declara la API de RenderScene dentro del subsistema Rendering de MonacoEngine3.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class Skybox;

class
	RenderScene {
public:
	void clear();

public:
	std::vector<RenderObject> opaqueObjects;
	std::vector<RenderObject> transparentObjects;
	std::vector<LightData> directionalLights;
	Skybox* skybox = nullptr;
};