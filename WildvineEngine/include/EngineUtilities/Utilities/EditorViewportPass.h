/**
 * @file EditorViewportPass.h
 * @brief Declara la API de EditorViewportPass dentro del subsistema Utilities de WildvineEngine.
 * @ingroup utilities
 */
#pragma once
#include "Prerequisites.h"
#include "Texture.h"
#include "RenderTargetView.h"
#include "DepthStencilView.h"

class Device;
class DeviceContext;

/**
 * @class EditorViewportPass
 * @brief Encapsula un pase de renderizado fuera de pantalla (off-screen).
 *
 * Esta clase administra su propio Render Target y Depth Stencil, permitiendo
 * renderizar una escena completa en una textura que posteriormente puede ser
 * leída por un shader o mostrada en la interfaz gráfica del editor (ImGui).
 */
class
	EditorViewportPass {
public:
	/**
	 * @brief Constructor por defecto.
	 */
	EditorViewportPass() = default;

	/**
	 * @brief Destructor por defecto.
	 */
	~EditorViewportPass() = default;

	/**
	 * @brief Inicializa los recursos de textura y vistas para el pase de renderizado.
	 * @param device Referencia al dispositivo gráfico.
	 * @param width Ancho inicial en píxeles.
	 * @param height Alto inicial en píxeles.
	 * @return HRESULT Código de éxito o error tras la creación de los recursos.
	 */
	HRESULT init(Device& device, unsigned int width, unsigned int height);

	/**
	 * @brief Recrea los recursos internos para ajustarse a una nueva resolución.
	 * @param device Referencia al dispositivo gráfico.
	 * @param width Nuevo ancho en píxeles.
	 * @param height Nuevo alto en píxeles.
	 * @return HRESULT Código de éxito o error tras el redimensionado.
	 */
	HRESULT resize(Device& device, unsigned int width, unsigned int height);

	/**
	 * @brief Establece este pase como el objetivo activo en el pipeline y limpia los buffers.
	 * @param deviceContext Contexto del dispositivo para emitir los comandos.
	 * @param clearColor Arreglo de 4 flotantes (RGBA) con el color de limpieza.
	 */
	void begin(DeviceContext& deviceContext, const float clearColor[4]);

	/**
	 * @brief Intercambia eficientemente los recursos internos con otro pase.
	 * Útil para técnicas de post-procesamiento (ping-pong rendering).
	 * @param other Referencia al pase con el que se intercambiarán los datos.
	 */
	void swap(EditorViewportPass& other);

	/**
	 * @brief Limpia únicamente el buffer de profundidad/esténcil de este pase.
	 * @param deviceContext Contexto del dispositivo.
	 */
	void clearDepth(DeviceContext& deviceContext);

	/**
	 * @brief Aplica la configuración de viewport de este pase al contexto del dispositivo.
	 * @param deviceContext Contexto del dispositivo.
	 */
	void setViewport(DeviceContext& deviceContext);

	/**
	 * @brief Libera todos los recursos gráficos (texturas y vistas) asociados a este pase.
	 */
	void destroy();

	/**
	 * @brief Obtiene la vista de recurso de shader (SRV) del color renderizado.
	 * @return ID3D11ShaderResourceView* Puntero a la textura final del pase.
	 */
	ID3D11ShaderResourceView* getSRV() const { return m_colorSRV.m_textureFromImg; }

	/**
	 * @brief Obtiene el ancho actual del pase de renderizado.
	 * @return unsigned int Ancho en píxeles.
	 */
	unsigned int getWidth() const { return m_width; }

	/**
	 * @brief Obtiene el alto actual del pase de renderizado.
	 * @return unsigned int Alto en píxeles.
	 */
	unsigned int getHeight() const { return m_height; }

	/**
	 * @brief Comprueba si los recursos vitales del pase fueron creados y son válidos.
	 * @return bool True si las texturas base y las vistas existen.
	 */
	bool isValid() const
	{
		return m_colorTexture.m_texture != nullptr &&
			m_colorSRV.m_textureFromImg != nullptr &&
			m_depthTexture.m_texture != nullptr;
	}

private:
	/**
	 * @brief Método interno que centraliza la creación de los recursos en memoria de video.
	 * @param device Dispositivo gráfico.
	 * @param width Ancho objetivo.
	 * @param height Alto objetivo.
	 * @return HRESULT S_OK si se instancian correctamente todas las texturas y vistas.
	 */
	HRESULT createResources(Device& device, unsigned int width, unsigned int height);

private:
	Texture           m_colorTexture;   ///< Textura que almacena la información de color (RGB/RGBA).
	Texture           m_colorSRV;       ///< Vista para leer la textura de color desde un shader.
	RenderTargetView  m_rtv;            ///< Vista para enlazar la textura como destino de escritura gráfico.

	Texture           m_depthTexture;   ///< Textura que almacena la información de profundidad espacial.
	DepthStencilView  m_dsv;            ///< Vista para enlazar la textura de profundidad al pipeline.

	unsigned int      m_width = 1;      ///< Ancho en píxeles del viewport actual.
	unsigned int      m_height = 1;     ///< Alto en píxeles del viewport actual.
};