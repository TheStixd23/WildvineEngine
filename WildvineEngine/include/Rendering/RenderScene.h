#pragma once
#include "Prerequisites.h"
#include "Rendering/RenderTypes.h"

class Skybox;

/**
 * @class RenderScene
 * @brief Contenedor que agrupa y clasifica los datos visibles de la escena para una pasada de renderizado.
 *
 * RenderScene recolecta y organiza de forma óptima todos los elementos que la cámara puede ver
 * en un cuadro (frame) actual. Clasifica los objetos según su tipo de material (opacos vs transparentes)
 * y almacena las luces y el entorno (Skybox) para que el sistema de renderizado pueda procesarlos
 * eficientemente sin desperdiciar ciclos de GPU.
 */
class RenderScene {
public:
    /**
     * @brief Limpia por completo todos los contenedores de la escena para el siguiente frame.
     * * Este método vacía las listas de objetos opacos, transparentes y luces, y restablece
     * el puntero del Skybox. Se invoca típicamente al inicio de cada frame para comenzar
     * una nueva recolección de datos (frustum culling).
     */
    void clear();

public:
    /** * @brief Lista de objetos opacos visibles en el frame actual.
     * @note Se renderizan de adelante hacia atrás (Front-to-Back) para maximizar el Early-Z testing
     * y evitar el sobre-sombreado (overdraw).
     */
    std::vector<RenderObject> opaqueObjects;

    /** * @brief Lista de objetos transparentes o translúcidos visibles en el frame actual.
     * @note Se deben renderizar de atrás hacia adelante (Back-to-Front) después de los objetos opacos
     * para asegurar una correcta mezcla de colores (Alpha Blending).
     */
    std::vector<RenderObject> transparentObjects;

    /** * @brief Colección de luces direccionales que afectan a la escena (e.g., el Sol).
     * @note Estas luces no tienen una posición definida, solo dirección, e iluminan de forma infinita.
     */
    std::vector<LightData> directionalLights;

    /** * @brief Puntero al Skybox (caja de cielo/entorno) activo en la escena.
     * Puede ser nulo (`nullptr`) si la escena no cuenta con un fondo texturizado.
     */
    Skybox* skybox = nullptr;
};