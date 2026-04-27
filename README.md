# 🌿 Wildvine Engine

**WildvineEngine** es un motor gráfico 3D de alto rendimiento desarrollado en **C++** y **DirectX 11**.  
Este proyecto sirve como compendio práctico y arquitectónico para la materia de **Gráficas Computacionales 3D (Generación 2026-01)**.

El motor implementa una arquitectura moderna basada en un **Grafo de Escena (Scene Graph)**, un sistema base de **Entidades y Componentes (ECS)**, un robusto **Pipeline de Renderizado Forward** con soporte para sombras dinámicas, y un **Editor Visual** completo utilizando *Dear ImGui* e *ImGuizmo*.

---

## 📊 Arquitectura del Motor (Diagrama de Flujo)

```mermaid
graph TD
    A[wWinMain - Entry Point] --> B(BaseApp)
    B --> C{Sistemas Principales}
    
    C --> D[Render Pipeline DirectX 11]
    C --> E[Scene Graph / ECS]
    C --> F[Editor UI - GUI]
    C --> G[Gestión de Memoria API - EU]

    D --> D1[Forward Renderer & Shadow Pass]
    D --> D2[Render Queues: Opaque / Transparent]
    D --> D3[Skybox & Environment]

    E --> E1[Gestión de Entidades / Actores]
    E --> E2[Jerarquía recursiva Padre-Hijo]
    
    F --> F1[Outliner]
    F --> F2[Inspector]
    F --> F3[Viewport & ImGuizmo Toolbar]
````

---

## ⚙️ Sistemas y Características Principales

El motor está modularizado en varias utilidades y subsistemas fundamentales para garantizar escalabilidad y un alto rendimiento gráfico.

---

### 🧩 1. Sistema de Componentes (ECS Base)

El motor utiliza un enfoque orientado a datos para definir los objetos en pantalla.

| Tipo de Componente | Descripción                            |
| ------------------ | -------------------------------------- |
| **NONE**           | Tipo base o componente no inicializado |
| **TRANSFORM**      | Posición, rotación y escala            |
| **MESH**           | Datos de vértices e índices            |
| **MATERIAL**       | Shaders, texturas y estados de render  |
| **HIERARCHY**      | Relaciones dentro del Scene Graph      |

---

### 🌳 2. Scene Graph (Grafo de Escena)

* Relaciones **padre-hijo dinámicas**
* Transformaciones **locales y globales**
* Cálculo recursivo mediante `updateWorldRecursive`

---

### 🎮 3. Pipeline de Renderizado (Forward Renderer)

* Clasificación en:

  * `opaqueQueue` (Early-Z, front-to-back)
  * `transparentQueue` (Alpha Blend, back-to-front)
* **Shadow Mapping dinámico (2048x2048)**
* Uso de **Constant Buffers alineados a 16 bytes**

---

### 🌌 4. Sistema de Entornos (Skybox)

* Renderizado mediante **Cubemap**
* Eliminación de traslación en la **View Matrix**
* Depth configurado para renderizar siempre al fondo

---

### 🧠 5. Gestión de Memoria (Namespace EU)

* `TSharedPointer<T>` → conteo de referencias
* `TUniquePtr<T>` → propiedad única
* `TWeakPointer<T>` → evita ciclos de dependencia

---

### 🛠️ 6. Editor Visual (GUI)

Construido con **Dear ImGui** e **ImGuizmo**.

| Panel     | Función                  |
| --------- | ------------------------ |
| Dockspace | Organización de ventanas |
| Outliner  | Jerarquía de actores     |
| Inspector | Edición de propiedades   |
| Viewport  | Render en tiempo real    |
| ImGuizmo  | Manipulación 3D          |

---

## 🛠️ Tecnologías y Dependencias

| Tecnología     | Uso                    |
| -------------- | ---------------------- |
| **C++ 11+**    | Lenguaje principal     |
| **DirectX 11** | API gráfica            |
| **XNAMath**    | Matemáticas            |
| **FBX SDK**    | Importación de modelos |
| **Dear ImGui** | Interfaz               |
| **ImGuizmo**   | Manipulación 3D        |
| **stb_image**  | Carga de texturas      |

---

## 📂 Estructura de Directorios

```
WildvineEngine/
│
├── include/     # Headers (.h)
├── source/      # Implementación (.cpp)
├── lib/         # Librerías externas
└── Imgui/       # UI (ImGui + ImGuizmo)
```

---

## 🚀 Requisitos de Compilación e Instalación

### Requisitos

* Visual Studio 2019 o 2022
* Windows SDK
* Autodesk FBX SDK

### Pasos

1. Abrir la solución:

   ```
   WildvineEngine_2010.sln
   ```

2. Seleccionar configuración:

   * Debug / Release
   * x64

3. Compilar:

   ```
   Ctrl + Shift + B
   ```

4. Ejecutar 🚀

---

## 📌 Notas

> Proyecto desarrollado con fines educativos y de investigación en arquitectura de motores de videojuegos.

