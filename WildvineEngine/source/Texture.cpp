#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Texture.h"
#include "Device.h"
#include "DeviceContext.h"
#include <fstream>

namespace {
    bool FileExists(const std::string& path) {
        std::ifstream file(path.c_str(), std::ios::binary);
        return file.good();
    }

    bool HasKnownExtension(const std::string& path) {
        const size_t slash = path.find_last_of("/\\");
        const size_t dot = path.find_last_of('.');
        return dot != std::string::npos &&
            (slash == std::string::npos || dot > slash);
    }

    std::string ResolveTexturePath(
        const std::string& textureName,
        ExtensionType extensionType) {

        if (HasKnownExtension(textureName) && FileExists(textureName)) {
            return textureName;
        }

        std::vector<std::string> candidates;
        switch (extensionType) {
        case DDS:
            candidates.push_back(textureName + ".dds");
            break;
        case JPG:
            candidates.push_back(textureName + ".jpg");
            candidates.push_back(textureName + ".jpeg");
            break;
        case TGA:
            candidates.push_back(textureName + ".tga");
            break;
        case BMP:
            candidates.push_back(textureName + ".bmp");
            break;
        case PNG:
        default:
            candidates.push_back(textureName + ".png");
            break;
        }

        for (const std::string& candidate : candidates) {
            if (FileExists(candidate)) return candidate;
        }

        return candidates.empty() ? textureName : candidates.front();
    }
}

HRESULT Texture::init(
    Device& device,
    const std::string& textureName,
    ExtensionType extensionType) {

    if (!device.m_device) {
        ERROR("Texture", "init", "Device is null.");
        return E_POINTER;
    }
    if (textureName.empty()) {
        ERROR("Texture", "init", "Texture name cannot be empty.");
        return E_INVALIDARG;
    }

    destroy();
    m_textureName = ResolveTexturePath(textureName, extensionType);

    if (!FileExists(m_textureName)) {
        ERROR("Texture", "init", "No se encontro la textura: " << m_textureName.c_str());
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (extensionType == DDS) {
        const HRESULT hr = D3DX11CreateShaderResourceViewFromFile(
            device.m_device,
            m_textureName.c_str(),
            nullptr,
            nullptr,
            &m_textureFromImg,
            nullptr);

        if (FAILED(hr)) {
            ERROR("Texture", "init", "Failed to load DDS texture: " << m_textureName.c_str());
        }
        return hr;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* pixels = stbi_load(
        m_textureName.c_str(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha);

    if (!pixels || width <= 0 || height <= 0) {
        const char* reason = stbi_failure_reason();
        ERROR("Texture", "init",
            "No se pudo cargar la textura: " << m_textureName.c_str()
            << " | " << (reason ? reason : "error desconocido"));
        if (pixels) stbi_image_free(pixels);
        return E_FAIL;
    }

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = static_cast<UINT>(width);
    textureDesc.Height = static_cast<UINT>(height);
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = pixels;
    initialData.SysMemPitch = static_cast<UINT>(width * 4);

    HRESULT hr = device.CreateTexture2D(
        &textureDesc,
        &initialData,
        &m_texture);

    stbi_image_free(pixels);

    if (FAILED(hr)) {
        ERROR("Texture", "init", "No se pudo crear ID3D11Texture2D.");
        destroy();
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device.m_device->CreateShaderResourceView(
        m_texture,
        &srvDesc,
        &m_textureFromImg);

    // El SRV conserva una referencia interna al recurso.
    SAFE_RELEASE(m_texture);

    if (FAILED(hr)) {
        ERROR("Texture", "init", "No se pudo crear el ShaderResourceView.");
        destroy();
        return hr;
    }

    MESSAGE("Texture", "init", "Textura cargada: " << m_textureName.c_str());
    return S_OK;
}

HRESULT Texture::init(
    Device& device,
    unsigned int width,
    unsigned int height,
    DXGI_FORMAT format,
    unsigned int bindFlags,
    unsigned int sampleCount,
    unsigned int qualityLevels) {

    if (!device.m_device) {
        ERROR("Texture", "init", "Device is null.");
        return E_POINTER;
    }
    if (width == 0 || height == 0) {
        ERROR("Texture", "init", "Width and height must be greater than 0.");
        return E_INVALIDARG;
    }

    destroy();

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = sampleCount;
    desc.SampleDesc.Quality = qualityLevels;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = bindFlags;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    const HRESULT hr = device.CreateTexture2D(&desc, nullptr, &m_texture);
    if (FAILED(hr)) {
        ERROR("Texture", "init", "Failed to create texture with specified params.");
    }
    return hr;
}

HRESULT Texture::init(Device& device, Texture& textureRef, DXGI_FORMAT format) {
    if (!device.m_device) {
        ERROR("Texture", "init", "Device is null.");
        return E_POINTER;
    }
    if (!textureRef.m_texture) {
        ERROR("Texture", "init", "Texture is null.");
        return E_POINTER;
    }

    destroy();

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    const HRESULT hr = device.m_device->CreateShaderResourceView(
        textureRef.m_texture,
        &srvDesc,
        &m_textureFromImg);

    if (FAILED(hr)) {
        ERROR("Texture", "init", "Failed to create shader resource view.");
    }
    return hr;
}

void Texture::update() {
}

void Texture::render(
    DeviceContext& deviceContext,
    unsigned int startSlot,
    unsigned int numViews) {

    if (!deviceContext.m_deviceContext) {
        ERROR("Texture", "render", "Device Context is null.");
        return;
    }

    if (m_textureFromImg) {
        deviceContext.PSSetShaderResources(
            startSlot,
            numViews,
            &m_textureFromImg);
    }
}

void Texture::destroy() {
    SAFE_RELEASE(m_texture);
    SAFE_RELEASE(m_textureFromImg);
    m_textureName.clear();
}

HRESULT Texture::CreateCubemap(
    Device& device,
    DeviceContext& deviceContext,
    const std::array<std::string, 6>& facePaths,
    bool generateMips) {

    if (!device.m_device || !deviceContext.m_deviceContext) {
        return E_POINTER;
    }

    destroy();
    stbi_set_flip_vertically_on_load(false);

    int width = 0;
    int height = 0;
    std::array<unsigned char*, 6> facePixels{};
    facePixels.fill(nullptr);

    auto releasePixels = [&]() {
        for (unsigned char*& pixels : facePixels) {
            if (pixels) {
                stbi_image_free(pixels);
                pixels = nullptr;
            }
        }
    };

    for (int face = 0; face < 6; ++face) {
        int faceWidth = 0;
        int faceHeight = 0;
        int channels = 0;
        facePixels[face] = stbi_load(
            facePaths[face].c_str(),
            &faceWidth,
            &faceHeight,
            &channels,
            STBI_rgb_alpha);

        if (!facePixels[face]) {
            ERROR("Texture", "CreateCubemap", "No se pudo cargar una cara del cubemap.");
            releasePixels();
            return E_FAIL;
        }

        if (face == 0) {
            width = faceWidth;
            height = faceHeight;
        }
        else if (faceWidth != width || faceHeight != height) {
            ERROR("Texture", "CreateCubemap", "Todas las caras deben tener el mismo tamano.");
            releasePixels();
            return E_INVALIDARG;
        }
    }

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = static_cast<UINT>(width);
    textureDesc.Height = static_cast<UINT>(height);
    textureDesc.MipLevels = generateMips ? 0 : 1;
    textureDesc.ArraySize = 6;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE |
        (generateMips ? D3D11_BIND_RENDER_TARGET : 0);
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE |
        (generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);

    HRESULT hr = S_OK;
    if (!generateMips) {
        std::array<D3D11_SUBRESOURCE_DATA, 6> initialData{};
        for (int face = 0; face < 6; ++face) {
            initialData[face].pSysMem = facePixels[face];
            initialData[face].SysMemPitch = static_cast<UINT>(width * 4);
        }
        hr = device.CreateTexture2D(&textureDesc, initialData.data(), &m_texture);
    }
    else {
        hr = device.CreateTexture2D(&textureDesc, nullptr, &m_texture);
        if (SUCCEEDED(hr)) {
            UINT mipCount = 1;
            UINT largestDimension = static_cast<UINT>((width > height) ? width : height);
            while (largestDimension > 1) {
                largestDimension >>= 1;
                ++mipCount;
            }

            for (UINT face = 0; face < 6; ++face) {
                const UINT subresource = D3D11CalcSubresource(0, face, mipCount);
                deviceContext.UpdateSubresource(
                    m_texture,
                    subresource,
                    nullptr,
                    facePixels[face],
                    static_cast<UINT>(width * 4),
                    0);
            }
        }
    }

    releasePixels();

    if (FAILED(hr)) {
        destroy();
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = generateMips ? static_cast<UINT>(-1) : 1;

    hr = device.m_device->CreateShaderResourceView(
        m_texture,
        &srvDesc,
        &m_textureFromImg);

    if (FAILED(hr)) {
        destroy();
        return hr;
    }

    if (generateMips) {
        deviceContext.m_deviceContext->GenerateMips(m_textureFromImg);
    }

    m_textureName = "Cubemap";
    return S_OK;
}
