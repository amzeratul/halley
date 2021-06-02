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
	if (srvAlt) {
		srvAlt->Release();
		srvAlt = nullptr;
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
	srvAlt = other.srvAlt;
	samplerState = other.samplerState;
	format = other.format;
	size = other.size;

	other.texture = nullptr;
	other.srv = nullptr;
	other.srvAlt = nullptr;
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

void DX11Texture::doLoad(TextureDescriptor& descriptor)
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
	case TextureFormat::Depth:
		desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		bpp = 4;
		break;
	default:
		throw Exception("Unknown texture format", HalleyExceptions::VideoPlugin);
	}

	desc.BindFlags = 0;
	if (descriptor.isDepthStencil) {
		desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	} else if (!descriptor.canBeReadOnCPU) {
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
	} else {
		if (descriptor.canBeUpdated) {
			desc.Usage = D3D11_USAGE_DEFAULT;
		} else {
			desc.Usage = D3D11_USAGE_IMMUTABLE;
		}
		subResData.pSysMem = descriptor.pixelData.getSpan().data();
		subResData.SysMemPitch = descriptor.pixelData.getStrideOr(bpp * size.x);
		subResData.SysMemSlicePitch = subResData.SysMemPitch;
		res = &subResData;
	}

	desc.CPUAccessFlags = 0;
	if (descriptor.canBeReadOnCPU) {
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.Usage = D3D11_USAGE_STAGING;
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

		if (descriptor.format == TextureFormat::Depth) {
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			result = video.getDevice().CreateShaderResourceView(texture, &srvDesc, &srv);
			if (result != S_OK) {
				throw Exception("Error creating shader resource view", HalleyExceptions::VideoPlugin);
			}

			srvDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
			result = video.getDevice().CreateShaderResourceView(texture, &srvDesc, &srvAlt);
			if (result != S_OK) {
				throw Exception("Error creating shader resource view", HalleyExceptions::VideoPlugin);
			}
		} else {
			result = video.getDevice().CreateShaderResourceView(texture, &srvDesc, &srv);
			if (result != S_OK) {
				throw Exception("Error creating shader resource view", HalleyExceptions::VideoPlugin);
			}
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

void DX11Texture::bind(DX11Video& video, int textureUnit, TextureSamplerType samplerType) const
{
	waitForLoad();

	ID3D11ShaderResourceView* srvs[] = { samplerType == TextureSamplerType::Stencil2D ? srvAlt : srv };
	ID3D11SamplerState* samplers[] = { samplerState };
	video.getDeviceContext().PSSetShaderResources(textureUnit, 1, srvs);
	video.getDeviceContext().PSSetSamplers(textureUnit, 1, samplers);
}

void DX11Texture::doCopyToTexture(Texture& otherRaw) const
{
	auto& other = dynamic_cast<DX11Texture&>(otherRaw);
	
	auto& dc = video.getDeviceContext();
	dc.CopyResource(other.texture, texture);
}

void DX11Texture::doCopyToImage(Image& image) const
{
	if (descriptor.canBeReadOnCPU) {
		copyToImageDirectly(image);
	} else {
		// Can't read on CPU, copy to a temporary texture first
		auto temp = video.createTexture(getSize());
		auto desc = TextureDescriptor(getSize(), TextureFormat::RGBA);
		desc.canBeReadOnCPU = true;
		temp->startLoading();
		temp->load(std::move(desc));
		doCopyToTexture(*temp);
		dynamic_cast<DX11Texture&>(*temp).copyToImageDirectly(image);
	}		
}

void DX11Texture::copyToImageDirectly(Image& image) const
{
	auto& dc = video.getDeviceContext();
	D3D11_MAPPED_SUBRESOURCE subResource;
	const HRESULT result = dc.Map(texture, 0, D3D11_MAP_READ, 0, &subResource);
	if (!SUCCEEDED(result)) {
		throw Exception("Unable to map texture for CPU reading", HalleyExceptions::VideoPlugin);
	}

	const size_t h = image.getSize().y;
	const size_t w = image.getSize().x;
	const char* src = static_cast<const char*>(subResource.pData);
	const auto dst = image.getPixels4BPP();
	for (size_t y = 0; y < h; ++y) {
		const auto* srcPx = reinterpret_cast<const uint32_t*>(src + y * subResource.RowPitch);
		const auto dstPx = dst.subspan(y * w, w);
		for (size_t x = 0; x < w; ++x) {
			dstPx[x] = srcPx[x];
		}
	}

	dc.Unmap(texture, 0);
}

DXGI_FORMAT DX11Texture::getFormat() const
{
	return format;
}

ID3D11Texture2D* DX11Texture::getTexture() const
{
	return texture;
}
