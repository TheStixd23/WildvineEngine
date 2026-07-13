#pragma once
#include "Prerequisites.h"

class Entity;
class DeviceContext;
class Camera;
class RenderScene;

/**
 * @class SceneGraph
 * @brief Gestiona la jerarquía espacial (relaciones Padre-Hijo) de las entidades del mundo.
 *
 * El Grafo de Escena organiza las entidades de forma lógica y geométrica. Su función principal
 * es propagar de forma recursiva las transformaciones (posición, rotación, escala) desde los nodos
 * padres hacia los hijos y recopilar (`gatherRenderScene`) eficientemente los objetos visibles
 * para el pipeline de renderizado.
 */
class SceneGraph {
public:
    /** @brief Constructor por defecto. */
    SceneGraph() = default;

    /** @brief Destructor por defecto. */
    ~SceneGraph() = default;

    /**
     * @brief Inicializa el grafo de escena y prepara las estructuras internas.
     */
    void init();

    /**
     * @brief Registra una nueva entidad huérfana (nodo raíz inicial) dentro del grafo de escena.
     * @param e Puntero a la entidad a registrar.
     */
    void addEntity(Entity* e);

    /**
     * @brief Elimina una entidad del grafo, gestionando su desvinculación y la de sus hijos.
     * @param e Puntero a la entidad a remover.
     */
    void removeEntity(Entity* e);

    /**
     * @brief Verifica si una entidad específica es ancestro (padre, abuelo, etc.) de otra en el árbol jerárquico.
     * * Es vital para prevenir dependencias cíclicas antes de hacer un enlace.
     * @param possibleAncestor Puntero al nodo que se sospecha que es el ancestro.
     * @param node Puntero al nodo hijo/descendiente a evaluar.
     * @return true Si `possibleAncestor` está en una jerarquía superior de `node`.
     * @return false Si no hay relación directa de herencia superior.
     */
    bool isAncestor(Entity* possibleAncestor, Entity* node) const;

    /**
     * @brief Enlaza una entidad como hija de otro nodo padre en el grafo.
     * * Automáticamente calcula y ajusta las transformaciones locales de la entidad hija
     * para mantener su coherencia visual en el espacio de mundo.
     * @param child Puntero a la entidad que se convertirá en subordinada.
     * @param parent Puntero a la entidad que actuará como contenedor/padre.
     * @return true Si el enlace fue exitoso.
     * @return false Si falló (e.g., si se detecta una herencia cíclica).
     */
    bool attach(Entity* child, Entity* parent);

    /**
     * @brief Desvincula un nodo hijo de su padre actual, regresándolo a la raíz del grafo.
     * @param child Puntero a la entidad que se desea independizar.
     * @return true Si se logró desvincular con éxito.
     * @return false Si la entidad no tenía un padre asignado o no pertenecía al grafo.
     */
    bool detach(Entity* child);

    /**
     * @brief Actualiza la lógica de las entidades y procesa las matrices de transformación del mundo.
     * * Llama internamente a `updateWorldRecursive` para calcular la posición global final de cada nodo.
     * @param deltaTime Tiempo transcurrido desde el último frame (segundos).
     * @param deviceContext Referencia al contexto gráfico (por si se requieren actualizaciones físicas directas).
     */
    void update(float deltaTime, DeviceContext& deviceContext);

    /**
     * @brief Ejecuta una pasada de renderizado directo sobre el grafo de la escena (si aplica).
     * @param deviceContext Referencia al contexto de hardware de la API gráfica.
     */
    void render(DeviceContext& deviceContext);

    /**
     * @brief Aplica frustum culling desde la perspectiva de una cámara y empaqueta los objetos visibles.
     * * Recorre el grafo, evalúa qué entidades están en el campo de visión de la `camera`
     * y llena las colas de renderizado en `outScene` para su posterior dibujado.
     * @param outScene Estructura RenderScene de salida que recolectará los objetos y luces.
     * @param camera Cámara que define la matriz de vista y el frustum de visibilidad.
     */
    void gatherRenderScene(RenderScene& outScene, const Camera& camera);

    /**
     * @brief Destruye el grafo, desvinculando todas las referencias de entidades y limpiando la memoria.
     */
    void destroy();

private:
    /**
     * @brief Método interno recursivo que computa la matriz de mundo real de un nodo multiplicándola por la de su padre.
     * * Resuelve la ecuación: $World_{Hijo} = Local_{Hijo} \times World_{Padre}$
     * @param node Nodo actual que se está procesando.
     * @param parentWorld Matriz global acumulada del padre.
     */
    void updateWorldRecursive(Entity* node, const XMMATRIX& parentWorld);

    /**
     * @brief Comprueba si una entidad es un nodo raíz (no posee ningún padre asignado).
     * @param e Puntero a la entidad evaluada.
     */
    bool isRoot(Entity* e) const;

    /**
     * @brief Verifica si una entidad ya se encuentra registrada en el contenedor del grafo.
     * @param e Puntero a la entidad evaluada.
     */
    bool isRegistered(Entity* e) const;

private:
    // std::vector<EU::TSharedPointer<Entity>> m_entities; ///< Código comentado: Futura migración a Smart Pointers.

public:
    /** * @brief Vector con todas las entidades registradas en el grafo.
     * @todo Se recomienda pasar a 'private' en el futuro para evitar modificaciones externas no controladas del árbol.
     */
    std::vector<Entity*> m_entities;
};