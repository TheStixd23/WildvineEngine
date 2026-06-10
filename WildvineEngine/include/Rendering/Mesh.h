/**
 * @file Mesh.h
 * @brief Declara la API de Mesh dentro del subsistema Rendering de WildvineEngine.
 * @ingroup rendering
 */
#pragma once
#include "Prerequisites.h"
#include "Buffer.h"

 /**
  * @struct Submesh
  * @brief Describe una porcion renderizable de una malla con sus buffers asociados.
  *
  * Una submalla representa un bloque de geometría contiguo que comparte un único
  * material dentro de un modelo 3D complejo.
  */
struct
	Submesh {
	Buffer vertexBuffer;          ///< Buffer de vertices de la submalla.
	Buffer indexBuffer;           ///< Buffer de indices de la submalla.
	unsigned int indexCount = 0;  ///< Numero de indices a dibujar.
	unsigned int startIndex = 0;  ///< Offset inicial dentro del index buffer.
	unsigned int materialSlot = 0;///< Slot de material esperado por el renderer.
};

/**
 * @class Mesh
 * @brief Agrupa una coleccion de submallas listas para ser renderizadas.
 * * Actúa como el contenedor principal en memoria para la geometría de un modelo,
 * permitiendo gestionar y liberar los buffers de todas sus partes de forma conjunta.
 */
class
	Mesh {
public:
	/**
	 * @brief Obtiene la colección de submallas que componen esta malla.
	 * @return std::vector<Submesh>& Referencia modificable a la lista de submallas.
	 */
	std::vector<Submesh>& getSubmeshes() { return m_submeshes; }

	/**
	 * @brief Obtiene la colección de submallas de solo lectura.
	 * @return const std::vector<Submesh>& Referencia constante a la lista de submallas.
	 */
	const std::vector<Submesh>& getSubmeshes() const { return m_submeshes; }

	/**
	 * @brief Libera todos los buffers asociados a las submallas en la memoria de video.
	 */
	void
		destroy() {
		for (Submesh& submesh : m_submeshes) {
			submesh.vertexBuffer.destroy();
			submesh.indexBuffer.destroy();
		}
		m_submeshes.clear();
	}

private:
	std::vector<Submesh> m_submeshes; ///< Arreglo interno que contiene todas las partes de la malla.
};