#include "dx11_texture.h"
#include "dx11_video.h"
#include "halley/core/graphics/texture_descriptor.h"
using namespace Halley;

DX11Texture::DX11Texture(DX11Video& video, Vector2i size)
	: Texture(size)
	, video(video)
{
	startLoading();
}

DX11Texture::~DX11Texture()
{
	if (srv) {
		srv->Release();
		srv = nullptr;
	}
	if (texture) {
		texture->Release();
		texture = nullptr;
	}
}

void DX11Texture::load(TextureDescriptor&& descriptor)
{
	int bpp = 0;

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = desc.ArraySize = 1;

	switch (descriptor.format) {
	case TextureFormat::RGB:
		throw Exception("RGB textures are not supported");
		break;
	case TextureFormat::RGBA:
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bpp = 4;
		break;
	case TextureFormat::DEPTH:
		desc.Format = DXGI_FORMAT_D32_FLOAT;
		bpp = 4;
		break;
	default:
		throw Exception("Unknown texture format");
	}
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subResData;
	subResData.pSysMem = descriptor.pixelData.getSpan().data();
	subResData.SysMemPitch = bpp * size.x;
	subResData.SysMemSlicePitch = subResData.SysMemPitch;

	HRESULT result = video.getDevice().CreateTexture2D(&desc, &subResData, &texture);
	if (result != S_OK) {
		throw Exception("Error loading texture.");
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	video.getDevice().CreateShaderResourceView(texture, &srvDesc, &srv);

	doneLoading();
}

ID3D11Texture2D* DX11Texture::getTexture() const
{
	waitForLoad();
	return texture;
}

ID3D11ShaderResourceView* DX11Texture::getShaderResourceView() const
{
	waitForLoad();
	return srv;
}
