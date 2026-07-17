#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class Skybox;

class RenderScene {
public:
    void clear();

    // Agrega una luz validando sus valores antes de enviarla al renderer.
    void addLight(const LightData& light);

    // Solo las luces direccionales y spot pueden usar el shadow map 2D actual.
    const LightData* findPrimaryShadowLight() const;

    size_t getActiveLightCount() const { return lights.size(); }

public:
    std::vector<RenderObject> opaqueObjects;
    std::vector<RenderObject> transparentObjects;

    // Todas las luces activas y saneadas de la escena.
    std::vector<LightData> lights;

    // Se conserva para compatibilidad con ForwardRenderer.
    std::vector<LightData> directionalLights;

    Skybox* skybox = nullptr;
};
