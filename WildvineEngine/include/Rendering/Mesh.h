/**
 * @file Mesh.h
 * @brief Declara la API de Mesh dentro del subsistema Rendering.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Buffer.h"

 /**
  * @struct Submesh
  * @brief Describe una porcion renderizable de una malla con sus buffers asociados.
  *
  * Una submalla representa un subconjunto de geometría dentro de una malla más grande,
  * típicamente separada para poder aplicarle un material distinto.
  */
struct Submesh {
	Buffer vertexBuffer;          ///< Buffer de vertices de la submalla (almacena posiciones, normales, UVs, etc. en la GPU).
	Buffer indexBuffer;           ///< Buffer de indices de la submalla (define la topología, generalmente triángulos).
	unsigned int indexCount = 0;  ///< Numero total de indices a dibujar para esta submalla.
	unsigned int startIndex = 0;  ///< Offset inicial dentro del index buffer (usado en llamadas como DrawIndexed).
	unsigned int materialSlot = 0;///< Índice del slot de material esperado por el renderer para pintar esta geometría.
};

/**
 * @class Mesh
 * @brief Agrupa una coleccion de submallas listas para ser renderizadas.
 * * Un `Mesh` actúa como un contenedor para una o más estructuras `Submesh`,
 * permitiendo representar modelos 3D complejos que requieren múltiples materiales.
 */
class Mesh {
public:
	/**
	 * @brief Obtiene una referencia modificable a la lista de submallas.
	 * @return Referencia al vector interno de estructuras `Submesh`.
	 */
	std::vector<Submesh>& getSubmeshes() { return m_submeshes; }

	/**
	 * @brief Obtiene una referencia de solo lectura a la lista de submallas.
	 * @return Referencia constante al vector interno de estructuras `Submesh`.
	 */
	const std::vector<Submesh>& getSubmeshes() const { return m_submeshes; }

	/**
	 * @brief Libera todos los buffers y recursos de hardware asociados a las submallas.
	 * * Recorre todas las submallas contenidas y destruye sus respectivos buffers
	 * de vértices e índices, para finalmente limpiar el vector contenedor.
	 */
	void destroy() {
		for (Submesh& submesh : m_submeshes) {
			submesh.vertexBuffer.destroy();
			submesh.indexBuffer.destroy();
		}
		m_submeshes.clear();
	}

private:
	std::vector<Submesh> m_submeshes; ///< Vector dinámico que contiene todas las partes renderizables (submallas) del modelo 3D.
};