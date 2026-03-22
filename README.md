# 🛠️ MonacoEngine — Motor Gráfico (Proyecto de Gráficas 3D)

## 📋 Resumen

`MonacoEngine` es una aplicación fundamental de **Direct3D 11** desarrollada en C++ como proyecto educativo. A diferencia de un motor completo, esta aplicación se centra en un único archivo (`MonacoEngine.cpp`) que maneja la inicialización, el bucle de renderizado y la lógica de la escena.

El proyecto renderiza un cubo 3D texturizado. Este cubo rota continuamente sobre su eje Y, mientras que su color de vértice se interpola dinámicamente usando funciones de seno y coseno para crear un efecto pulsante.

El motor utiliza un conjunto de clases auxiliares (`Window`, `Device`, `DeviceContext`, `SwapChain`, `Texture`, `ShaderProgram`) para encapsular y gestionar los objetos COM de Direct3D 11, simplificando la lógica en el archivo principal.

## 📌 Índice

  - [Resumen](https://www.google.com/search?q=%23-resumen)
  - [Objetivos del proyecto](https://www.google.com/search?q=%23-objetivos-del-proyecto)
  - [Arquitectura general](https://www.google.com/search?q=%23-arquitectura-general)
      - [Componentes principales](https://www.google.com/search?q=%23componentes-principales)
      - [Relaciones operativas](https://www.google.com/search?q=%23relaciones-operativas)
  - [Pipeline gráfico implementado](https://www.google.com/search?q=%23-pipeline-gr%C3%A1fico-implementado)
  - [Flujo de inicialización](https://www.google.com/search?q=%23-flujo-de-inicializaci%C3%B3n)
  - [Flujo de render (por-frame)](https://www.google.com/search?q=%23-flujo-de-render-por-frame)
  - [Clases / API clave](https://www.google.com/search?q=%23-clases--api-clave)

-----

## 🎯 Objetivos del proyecto

| Objetivo | Descripción |
|---|---|
| Arquitectura modular | Encapsular los componentes base de D3D11 (`Device`, `SwapChain`, `Texture`, etc.) en clases C++. |
| Comprensión de pipeline | Implementar el flujo completo de `InitDevice` y `Render` de D3D11 en un solo archivo para mayor claridad. |
| Render mínimo funcional | Mostrar un cubo 3D hard-coded, texturizado, rotando y cambiando de color. |
| Gestión de Shaders | Cargar y compilar shaders (VS/PS) desde un archivo (`.fx`) en tiempo de ejecución usando `D3DX11CompileFromFile`. |
| Gestión de recursos | Demostrar la creación de VBO, IBO, CBOs y carga de texturas, con su respectiva limpieza en `CleanupDevice`. |

-----

## 🏗 Arquitectura general

### Componentes principales

| Componente | Responsabilidad | API / Recursos Clave |
|---|---|---|
| **`MonacoEngine.cpp`** | Archivo principal. Contiene `wWinMain`, `InitDevice`, `Render` y `CleanupDevice`. Almacena todos los objetos gráficos como variables globales. | `g_device`, `g_swapChain`, `g_pVertexBuffer` |
| **`Window`** | Crea y maneja la ventana principal de Win32 (`HWND`). | `CreateWindow`, `WNDCLASSEX` |
| **`Device` / `DeviceContext`** | Envolturas (wrappers) ligeras para `ID3D11Device` e `ID3D11DeviceContext`. Proveen métodos para crear recursos (`CreateBuffer`, etc.). | `m_device`, `m_deviceContext` |
| **`SwapChain`** | Gestiona la creación de `D3D11CreateDevice` y `IDXGISwapChain`. Configura **4x MSAA**. | `D3D11CreateDevice`, `CreateSwapChain`, `Present` |
| **`ShaderProgram`** | Carga y compila shaders HLSL (`.fx`). Gestiona `ID3D11VertexShader`, `ID3D11PixelShader` y `ID3D11InputLayout`. | `D3DX11CompileFromFile`, `CreateVertexShader` |
| **`Texture`** | Clase para `ID3D11Texture2D`. Se usa para crear las texturas de RTV y DSV desde cero. | `CreateTexture2D` |
| **Recursos Globales** | Los buffers (`g_pVertexBuffer`, `g_pIndexBuffer`, CBOs) y la textura (`g_pTextureRV`) se manejan como variables globales en `MonacoEngine.cpp`, no a través de la clase `Buffer`. | `ID3D11Buffer*`, `ID3D11ShaderResourceView*` |

### Relaciones operativas

1.  `wWinMain` llama a `g_window.init()` para crear la ventana.
2.  `wWinMain` llama a `InitDevice()` para configurar todo el pipeline de D3D11.
3.  `InitDevice` usa las clases globales (`g_swapChain`, `g_renderTargetView`, `g_shaderProgram`, etc.) para inicializarlas una por una.
4.  `InitDevice` crea manualmente los VBO/IBO/CBOs del cubo y carga la textura `seafloor.dds`.
5.  El bucle principal (`wWinMain`) llama a `Render()` en cada fotograma.
6.  `Render` actualiza la matriz de mundo y el color, actualiza el CBO, bindea todos los recursos (shaders, buffers, textura, sampler) y llama a `g_deviceContext.DrawIndexed(36, 0, 0)`.
7.  `Render` finaliza llamando a `g_swapChain.present()` para mostrar el resultado.

-----

## 📷 Pipeline gráfico implementado

1.  **Inicialización (InitDevice):**
      * Se crea `Device`, `DeviceContext` y `SwapChain` (con **4x MSAA**).
      * Se crean `RenderTargetView` (para el back buffer) y `DepthStencilView` (con su propia textura).
2.  **Carga de Assets (InitDevice):**
      * Se compila `MonacoEngine.fx` para obtener VS y PS. El `InputLayout` se crea para `POSITION` y `TEXCOORD`.
      * Se crea un `g_pVertexBuffer` (24 vértices) y `g_pIndexBuffer` (36 índices) con la geometría de un cubo hard-coded.
      * Se cargan los buffers a la etapa de Input Assembler (`IASetVertexBuffers`, `IASetIndexBuffer`).
      * Se carga la textura `seafloor.dds` directamente usando `D3DX11CreateShaderResourceViewFromFile`.
      * Se crea un `g_pSamplerLinear` (Wrap).
3.  **Etapa de Vértice (Render):**
      * Se definen 3 Buffers de Constantes (CBOs): `CBNeverChanges` (Vista), `CBChangeOnResize` (Proyección), `CBChangesEveryFrame` (Mundo, Color).
      * En `Render`, se actualiza `g_World` (rotación) y `g_vMeshColor`.
      * Se actualiza `g_pCBChangesEveryFrame` con `UpdateSubresource`.
      * Se bindean los 3 CBOs al Vertex Shader (`VSSetConstantBuffers`).
4.  **Rasterización:**
      * Se establece la topología como `D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST`.
      * Se aplica el `Viewport`.
5.  **Shading (Etapa de Píxel):**
      * Se bindea `g_pCBChangesEveryFrame` (para el color) al Pixel Shader (`PSSetConstantBuffers`).
      * Se bindea la textura (`g_pTextureRV`) y el sampler (`g_pSamplerLinear`).
6.  **Salida (OM):**
      * Se limpian `RenderTargetView` (con color `0.1, 0.1, 0.1`) y `DepthStencilView`.
7.  **Dibujado y Presentación:**
      * Se llama a `g_deviceContext.DrawIndexed(36, 0, 0)`.
      * Se llama a `g_swapChain.present()`.

-----

## 🚀 Flujo de inicialización

1.  `g_window.init()` → Crea la ventana Win32.
2.  `g_swapChain.init()` → Crea `ID3D11Device`, `ID3D11DeviceContext`, `IDXGISwapChain` y configura 4x MSAA.
3.  `g_renderTargetView.init()` → Se enlaza al Back Buffer del SwapChain.
4.  `g_depthStencil.init()` → Crea la textura `ID3D11Texture2D` para el Depth Stencil.
5.  `g_depthStencilView.init()` → Crea el `ID3D11DepthStencilView` enlazado a la textura anterior.
6.  `g_viewport.init()` → Se configura con las dimensiones de la ventana.
7.  `g_shaderProgram.init("MonacoEngine.fx", ...)` → Compila VS/PS y crea el `InputLayout`.
8.  `g_device.m_device->CreateBuffer` → Crea `g_pVertexBuffer` (24 vértices) y `g_pIndexBuffer` (36 índices) para el cubo.
9.  `g_deviceContext.IASet...` → Bindea los VBO/IBO y establece la topología de triángulos.
10. `g_device.m_device->CreateBuffer` → Crea los 3 CBOs (`g_pCBNeverChanges`, `g_pCBChangeOnResize`, `g_pCBChangesEveryFrame`).
11. `D3DX11CreateShaderResourceViewFromFile("seafloor.dds", ...)` → Carga la textura `g_pTextureRV`.
12. `g_device.m_device->CreateSamplerState(...)` → Crea el `g_pSamplerLinear`.
13. Se inicializan las matrices `g_View` y `g_Projection` y se suben a sus CBOs correspondientes.

-----

## ⏱ Flujo de render (por frame)

1.  **(Update)** Se calcula el tiempo `t`. Se actualiza `g_World` (con `XMMatrixRotationY(t)`) y `g_vMeshColor` (con `sinf` y `cosf`).
2.  **(Render)** `g_renderTargetView.render()`: Limpia el RTV con color `(0.1, 0.1, 0.1, 1.0)` y bindea el RTV/DSV.
3.  `g_viewport.render()`: Establece el viewport.
4.  `g_depthStencilView.render()`: Limpia el DSV.
5.  `g_shaderProgram.render()`: Bindea VS, PS, e Input Layout.
6.  `g_deviceContext.UpdateSubresource(g_pCBChangesEveryFrame, ...)`: Sube la nueva matriz de mundo y el color al CBO.
7.  `g_deviceContext.VSSetConstantBuffers(...)`: Bindea los 3 CBOs a los slots 0, 1, 2 del VS.
8.  `g_deviceContext.PSSetConstantBuffers(...)`: Bindea el CBO de frame (slot 2) al PS.
9.  `g_deviceContext.PSSetShaderResources(...)`: Bindea la textura `g_pTextureRV` al slot 0.
10. `g_deviceContext.PSSetSamplers(...)`: Bindea el sampler `g_pSamplerLinear` al slot 0.
11. `g_deviceContext.DrawIndexed(36, 0, 0)`: Dibuja el cubo.
12. `g_swapChain.present()`: Muestra el frame en pantalla.

-----

## 📚 Clases / API clave

  * **`MonacoEngine.cpp`**: Contiene toda la lógica de orquestación, `InitDevice`, `Render`, `wWinMain`, y la gestión de recursos de la escena (buffers, texturas) como variables globales.
  * **`Device` / `DeviceContext`**: Clases wrapper esenciales que encapsulan `ID3D11Device` e `ID3D11DeviceContext`, proporcionando métodos de ayuda para la creación de recursos y la ejecución de comandos.
  * **`SwapChain`**: Clase crucial que maneja no solo el `IDXGISwapChain`, sino también la creación inicial del `Device` y `DeviceContext` de D3D11, incluyendo la configuración de 4x MSAA.
  * **`ShaderProgram`**: Gestiona la compilación de shaders HLSL (`.fx`) y la creación del `InputLayout` asociado.
  * **`Prerequisites.h`**: El archivo de cabecera más importante. Incluye todas las librerías necesarias (Windows, D3D11, D3DX11, XNA), define macros (`SAFE_RELEASE`, `MESSAGE`, `ERROR`) y las estructuras de datos clave: `SimpleVertex`, `CBNeverChanges`, `CBChangeOnResize`, `CBChangesEveryFrame`.


-----

## 📚 Diagrama uml
<img width="797" height="634" alt="Diagrama general" src="https://github.com/user-attachments/assets/6d01b8bf-7005-4cb4-961e-d4f3bc82003a" />


