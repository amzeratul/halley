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

static D3D11_TEXTURE_ADDRESS_MODE getAddressMode(TextureAddressMode mode)
{
	switch (mode) {
	case TextureAddressMode::Clamp:
		return D3D11_TEXTURE_ADDRESS_CLAMP;
	case TextureAddressMode::Mirror:
		return D3D11_TEXTURE_ADDRESS_MIRROR;
	case TextureAddressMode::Repeat:
		return D3D11_TEXTURE_ADDRESS_WRAP;
	}
	throw Exception("Unknown TextureAddressMode: " + toString(mode), HalleyExceptions::VideoPlugin);
}

void DX11Texture::load(TextureDescriptor&& descriptor)
{
	int bpp = 0;

	CD3D11_TEXTURE2D_DESC desc;
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = desc.ArraySize = 1;

	switch (descriptor.format) {
	case TextureFormat::Indexed:
	case TextureFormat::Red:
		desc.Format = DXGI_FORMAT_R8_UNORM;
		bpp = 1;
		break;
	case TextureFormat::RGB:
		throw Exception("RGB textures are not supported", HalleyExceptions::VideoPlugin);
		break;
	case TextureFormat::RGBA:
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bpp = 4;
		break;
	case TextureFormat::DEPTH:
		desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		bpp = 4;
		break;
	default:
		throw Exception("Unknown texture format", HalleyExceptions::VideoPlugin);
	}

	desc.BindFlags = 0;
	if (descriptor.isDepthStencil) {
		desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	} else {
		desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if (descriptor.isRenderTarget) {
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		}
	}

	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MiscFlags = 0;
	format = desc.Format;

	D3D11_SUBRESOURCE_DATA* res = nullptr;
	D3D11_SUBRESOURCE_DATA subResData;

	if (descriptor.pixelData.empty()) {
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
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
		throw Exception("Error loading texture.", HalleyExceptions::VideoPlugin);
	}

	if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
		CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;

		result = video.getDevice().CreateShaderResourceView(texture, &srvDesc, &srv);
		if (result != S_OK) {
			throw Exception("Error creating shader resource view", HalleyExceptions::VideoPlugin);
		}

		auto samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
		samplerDesc.Filter = descriptor.useFiltering ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = getAddressMode(descriptor.addressMode);
		result = video.getDevice().CreateSamplerState(&samplerDesc, &samplerState);
		if (result != S_OK) {
			throw Exception("Error creating sampler", HalleyExceptions::VideoPlugin);
		}
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
