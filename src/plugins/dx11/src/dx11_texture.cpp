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
	if (samplerState) {
		samplerState->Release();
		samplerState = nullptr;
	}
	if (srv) {
		srv->Release();
		srv = nullptr;
	}
	if (texture) {
		texture->Release();
		texture = nullptr;
	}
}

DX11Texture& DX11Texture::operator=(DX11Texture&& other) noexcept
{
	other.waitForLoad();

	texture = other.texture;
	srv = other.srv;
	samplerState = other.samplerState;
	format = other.format;
	size = other.size;

	other.texture = nullptr;
	other.srv = nullptr;
	other.samplerState = nullptr;

	doneLoading();

	return *this;
}

void DX11Texture::load(TextureDescriptor&& descriptor)
{
	int bpp = 0;

	CD3D11_TEXTURE2D_DESC desc;
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	switch (descriptor.format) {
	case TextureFormat::Indexed:
		desc.Format = DXGI_FORMAT_R8_UNORM;
		bpp = 1;
		break;
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
	if (descriptor.isRenderTarget) {
		desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}
	if (descriptor.isDepthStencil) {
		desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	}
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MiscFlags = 0;
	format = desc.Format;

	D3D11_SUBRESOURCE_DATA* res = nullptr;
	D3D11_SUBRESOURCE_DATA subResData;

	if (descriptor.pixelData.empty()) {
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	} else {
		if (descriptor.canBeUpdated) {
			desc.Usage = D3D11_USAGE_DEFAULT;
		} else {
			desc.Usage = D3D11_USAGE_IMMUTABLE;
		}
		desc.CPUAccessFlags = 0;
		subResData.pSysMem = descriptor.pixelData.getSpan().data();
		subResData.SysMemPitch = descriptor.pixelData.getStrideOr(bpp * size.x);
		subResData.SysMemSlicePitch = subResData.SysMemPitch;
		res = &subResData;
	}

	HRESULT result = video.getDevice().CreateTexture2D(&desc, res, &texture);
	if (result != S_OK) {
		throw Exception("Error loading texture.");
	}

	CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = video.getDevice().CreateShaderResourceView(texture, &srvDesc, &srv);
	if (result != S_OK) {
		throw Exception("Error creating shader resource view");
	}

	auto samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
	samplerDesc.Filter = descriptor.useFiltering ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = descriptor.clamp ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
	result = video.getDevice().CreateSamplerState(&samplerDesc, &samplerState);
	if (result != S_OK) {
		throw Exception("Error creating sampler");
	}

	doneLoading();
}

void DX11Texture::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<DX11Texture&>(resource));
}

void DX11Texture::bind(DX11Video& video, int textureUnit) const
{
	waitForLoad();

	ID3D11ShaderResourceView* srvs[] = { srv };
	ID3D11SamplerState* samplers[] = { samplerState };
	video.getDeviceContext().PSSetShaderResources(textureUnit, 1, srvs);
	video.getDeviceContext().PSSetSamplers(textureUnit, 1, samplers);
}

DXGI_FORMAT DX11Texture::getFormat() const
{
	return format;
}

ID3D11Texture2D* DX11Texture::getTexture() const
{
	return texture;
}
