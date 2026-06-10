/**
 * @file LayoutBuilder.h
 * @brief Declara la API de LayoutBuilder dentro del subsistema Utilities.
 * @ingroup utilities
 */
#pragma once
#include "Prerequisites.h"

 /**
  * @class LayoutBuilder
  * @brief Constructor fluido (Builder) para facilitar la creacion de descripciones de Input Layout en DirectX 11.
  * * Permite encadenar llamadas para configurar de forma limpia y legible los atributos
  * de los vértices (posición, normales, coordenadas de textura, etc.) o datos de instancia
  * que serán enviados al Input Assembler del pipeline gráfico.
  */
class
    LayoutBuilder {
public:
    /**
     * @brief Añade un nuevo elemento base (por defecto per-vertex) a la descripción del layout.
     * @param semantic Nombre de la semántica HLSL asociada (ej. "POSITION", "TEXCOORD").
     * @param format Formato de datos del elemento (ej. DXGI_FORMAT_R32G32B32_FLOAT).
     * @param semanticIndex Índice de la semántica para elementos repetidos (por defecto 0).
     * @param inputSlot Ranura del Input Assembler donde se vincularán los datos (por defecto 0).
     * @param alignedByteOffset Desplazamiento en bytes desde el inicio del vértice (por defecto auto-alineado).
     * @param slotClass Clasificación de los datos (por vértice o por instancia).
     * @param instanceStepRate Número de instancias a dibujar antes de avanzar un elemento (por defecto 0).
     * @return LayoutBuilder& Referencia a la propia instancia para permitir el encadenamiento de llamadas.
     */
     // **Add() base** (per-vertex por defecto)
    LayoutBuilder&
        Add(const char* semantic,
            DXGI_FORMAT format,
            UINT semanticIndex = 0,
            UINT inputSlot = 0,
            UINT alignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
            D3D11_INPUT_CLASSIFICATION slotClass = D3D11_INPUT_PER_VERTEX_DATA,
            UINT instanceStepRate = 0) {
        D3D11_INPUT_ELEMENT_DESC d{};
        d.SemanticName = semantic;
        d.SemanticIndex = semanticIndex;
        d.Format = format;
        d.InputSlot = inputSlot;
        d.AlignedByteOffset = alignedByteOffset;
        d.InputSlotClass = slotClass;
        d.InstanceDataStepRate = instanceStepRate;
        m_elems.push_back(d);
        return *this;
    }

    /**
     * @brief Atajo para añadir un elemento configurado específicamente para instanciación (Instancing).
     * @param semantic Nombre de la semántica HLSL asociada.
     * @param format Formato de datos del elemento.
     * @param semanticIndex Índice de la semántica (por defecto 0).
     * @param inputSlot Ranura del Input Assembler (por defecto 1, comúnmente usado para buffers de instancias).
     * @param alignedByteOffset Desplazamiento en bytes (por defecto auto-alineado).
     * @param instanceStepRate Número de instancias a dibujar antes de avanzar (por defecto 1).
     * @return LayoutBuilder& Referencia a la propia instancia para permitir el encadenamiento de llamadas.
     */
     // **Atajo** para instancing
    LayoutBuilder&
        AddInstance(const char* semantic,
            DXGI_FORMAT format,
            UINT semanticIndex = 0,
            UINT inputSlot = 1,
            UINT alignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT,
            UINT instanceStepRate = 1) {
        return Add(semantic, format, semanticIndex, inputSlot, alignedByteOffset,
            D3D11_INPUT_PER_INSTANCE_DATA, instanceStepRate);
    }

    /**
     * @brief Obtiene la colección construida de descripciones de elementos.
     * @return const std::vector<D3D11_INPUT_ELEMENT_DESC>& Referencia de solo lectura al arreglo interno.
     */
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& Get() const { return m_elems; }

    /**
     * @brief Obtiene la cantidad total de elementos registrados en el layout.
     * @return UINT Número de elementos construidos.
     */
    UINT Count() const { return (UINT)m_elems.size(); }

private:
    std::vector<D3D11_INPUT_ELEMENT_DESC> m_elems; ///< Vector interno que almacena las descripciones de los elementos de entrada D3D11.
};