#include "dx11_texture.h"
#include "dx11_video.h"
#include "halley/graphics/texture_descriptor.h"
using namespace Halley;

DX11Texture::DX11Texture(DX11Video& video, Vector2i size)
	: Texture(size)
	, video(video)
{
	startLoading();
}

DX11Texture::DX11Texture(DX11Video& video, Vector2i size, ID3D11ShaderResourceView* srv)
	: Texture(size)
	, video(video)
	, srv(srv)
{
	auto samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	for (int i = 0; i < 4; ++i) {
		samplerDesc.BorderColor[i] = 0.0f;
	}
	const auto result = video.getDevice().CreateSamplerState(&samplerDesc, &samplerState);
	if (result != S_OK) {
		throw Exception("Error creating sampler", HalleyExceptions::VideoPlugin);
	}

	format = DXGI_FORMAT_R8G8B8A8_UNORM;

	doneLoading();
}

DX11Texture::~DX11Texture()
{
	clear();
}

DX11Texture& DX11Texture::operator=(DX11Texture&& other) noexcept
{
	other.waitForLoad(true);

	texture = other.texture;
	srv = other.srv;
	srvAlt = other.srvAlt;
	samplerState = other.samplerState;
	format = other.format;
	size = other.size;
	descriptor = std::move(other.descriptor);

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
	case TextureAddressMode::Border:
		return D3D11_TEXTURE_ADDRESS_BORDER;
	}
	throw Exception("Unknown TextureAddressMode: " + toString(mode), HalleyExceptions::VideoPlugin);
}

void DX11Texture::doLoad(TextureDescriptor& descriptor)
{
	clear();

	int bpp = TextureDescriptor::getBytesPerPixel(descriptor.format);

	CD3D11_TEXTURE2D_DESC desc;
	desc.Width = size.x;
	desc.Height = size.y;
	desc.MipLevels = descriptor.useMipMap ? 0 : 1;
	desc.ArraySize = 1;

	switch (descriptor.format) {
	case TextureFormat::Indexed:
	case TextureFormat::Red:
		desc.Format = DXGI_FORMAT_R8_UNORM;
		break;
	case TextureFormat::RGB:
		throw Exception("RGB textures are not supported", HalleyExceptions::VideoPlugin);
		break;
	case TextureFormat::RGBA:
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case TextureFormat::Depth:
		desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		break;
	case TextureFormat::BGR565:
		desc.Format = DXGI_FORMAT_B5G6R5_UNORM;
		break;
	case TextureFormat::BGRA5551:
		desc.Format = DXGI_FORMAT_B5G5R5A1_UNORM;
		break;
	case TextureFormat::BGRX:
		desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
		break;
	case TextureFormat::SRGBA:
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		break;
	case TextureFormat::RGBAFloat16:
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	default:
		throw Exception("Unknown texture format", HalleyExceptions::VideoPlugin);
	}

	vramUsage = bpp * size.x * size.y;

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
	if (descriptor.useMipMap) {
		desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	}

	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MiscFlags = descriptor.useMipMap ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
	format = desc.Format;

	bool hasPixelData = false;
	D3D11_SUBRESOURCE_DATA subResData;
			
	if (descriptor.pixelData.empty()) {
		desc.Usage = D3D11_USAGE_DEFAULT;
	} else {
		if (descriptor.canBeUpdated || descriptor.useMipMap) {
			desc.Usage = D3D11_USAGE_DEFAULT;
		} else {
			desc.Usage = D3D11_USAGE_IMMUTABLE;
		}
		subResData.pSysMem = descriptor.pixelData.getSpan().data();
		subResData.SysMemPitch = descriptor.pixelData.getStrideOr(bpp * size.x);
		subResData.SysMemSlicePitch = 0;
		hasPixelData = true;
	}

	desc.CPUAccessFlags = 0;
	if (descriptor.canBeReadOnCPU) {
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.Usage = D3D11_USAGE_STAGING;
	}

	HRESULT result = video.getDevice().CreateTexture2D(&desc, hasPixelData && !descriptor.useMipMap ? &subResData : nullptr, &texture);
	if (result != S_OK) {
		throw Exception("Error loading texture.", HalleyExceptions::VideoPlugin);
	}

	if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
		CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = descriptor.useMipMap ? -1 : 1;
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
		
		if (descriptor.useMipMap) {
			if (hasPixelData) {
				video.getDeviceContext().UpdateSubresource(texture, 0, nullptr, subResData.pSysMem, subResData.SysMemPitch, 0);
			}
			generateMipMaps();
		}

		auto samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
		samplerDesc.Filter = descriptor.useFiltering ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = getAddressMode(descriptor.addressMode);
		for (int i = 0; i < 4; ++i) {
			samplerDesc.BorderColor[i] = 0.0f;
		}
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

void DX11Texture::generateMipMaps()
{
	if (descriptor.useMipMap && srv) {
		video.getDeviceContext().GenerateMips(srv);
	}
}

void DX11Texture::doCopyToTexture(Painter& painter, Texture& otherRaw) const
{
	auto& other = dynamic_cast<DX11Texture&>(otherRaw);
	
	auto& dc = video.getDeviceContext();
	dc.CopyResource(other.texture, texture);
}

void DX11Texture::doCopyToImage(Painter& painter, Image& image) const
{
	if (descriptor.canBeReadOnCPU) {
		copyToImageDirectly(image);
	} else {
		// Can't read on CPU, copy to a temporary texture first
		auto temp = video.createTexture(getSize());
		auto desc = TextureDescriptor(getSize(), descriptor.format);
		desc.canBeReadOnCPU = true;
		temp->startLoading();
		temp->load(std::move(desc));
		doCopyToTexture(painter, *temp);
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

	if (descriptor.format == TextureFormat::RGBA) {
		for (size_t y = 0; y < h; ++y) {
			const auto* srcPx = reinterpret_cast<const uint32_t*>(src + y * subResource.RowPitch);
			const auto dstPx = dst.subspan(y * w, w);
			for (size_t x = 0; x < w; ++x) {
				dstPx[x] = srcPx[x];
			}
		}
	} else if (descriptor.format == TextureFormat::BGR565) {
		for (size_t y = 0; y < h; ++y) {
			const auto* srcPx = reinterpret_cast<const uint16_t*>(src + y * subResource.RowPitch);
			const auto dstPx = dst.subspan(y * w, w);
			for (size_t x = 0; x < w; ++x) {
				const auto px = static_cast<uint32_t>(srcPx[x]);
				const auto b = (px & 0x001F) << 19;
				const auto g = (px & 0x07E0) << 5;
				const auto r = (px & 0xF800) >> 8;
				const auto a = static_cast<uint32_t>(0xFF) << 24;
				dstPx[x] = r | g | b | a;
			}
		}
	} else if (descriptor.format == TextureFormat::BGRX) {
		for (size_t y = 0; y < h; ++y) {
			const auto* srcPx = reinterpret_cast<const uint32_t*>(src + y * subResource.RowPitch);
			const auto dstPx = dst.subspan(y * w, w);
			for (size_t x = 0; x < w; ++x) {
				uint32_t px = srcPx[x];
				const uint32_t ag = px & 0xFF00FF00u;
				const uint32_t r = (px & 0x00FF0000u) >> 16;
				const uint32_t b = (px & 0x000000FFu) << 16;
				dstPx[x] = r | b | ag;
			}
		}
	} else {
		throw Exception("Unable to copy texture format to image: " + toString(descriptor.format), HalleyExceptions::VideoPlugin);
	}

	dc.Unmap(texture, 0);
}

void DX11Texture::clear()
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

DXGI_FORMAT DX11Texture::getFormat() const
{
	return format;
}

ID3D11Texture2D* DX11Texture::getTexture() const
{
	return texture;
}

void DX11Texture::replaceShaderResourceView(ID3D11ShaderResourceView* view)
{
	if (srv) {
		srv->Release();
	}
	srv = view;
}

size_t DX11Texture::getVRamUsage() const
{
	return vramUsage;
}
