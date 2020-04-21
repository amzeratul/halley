#include "metal_texture.h"
#include "metal_video.h"

using namespace Halley;

MetalTexture::MetalTexture(MetalVideo& video, Vector2i size)
	: Texture(size)
	, video(video)
{
	startLoading();
}

void MetalTexture::load(TextureDescriptor&& descriptor)
{
	MTLPixelFormat pixelFormat;
	int bytesPerPixel = 0;
	switch (descriptor.format) {
		case TextureFormat::Indexed:
		case TextureFormat::Red:
			pixelFormat = MTLPixelFormatR8Unorm;
			bytesPerPixel = 1;
			break;
		case TextureFormat::RGB:
			throw Exception("RGB textures are not supported", HalleyExceptions::VideoPlugin);
			break;
		case TextureFormat::RGBA:
			pixelFormat = MTLPixelFormatRGBA8Unorm;
			bytesPerPixel = 4;
			break;
		case TextureFormat::DEPTH:
			pixelFormat = MTLPixelFormatDepth32Float;
			bytesPerPixel = 4;
			break;
		default:
			throw Exception("Unknown texture format", HalleyExceptions::VideoPlugin);
	}

	auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
		width:descriptor.size.x
		height:descriptor.size.y
		mipmapped:descriptor.useMipMap
	];
	metalTexture = [video.getDevice() newTextureWithDescriptor:textureDescriptor];

	NSUInteger bytesPerRow = bytesPerPixel * descriptor.size.x;
	MTLRegion region = {
		{ 0, 0, 0 },
		{static_cast<NSUInteger>(descriptor.size.x), static_cast<NSUInteger>(descriptor.size.y), 1}
	};

	Byte* imageBytes;
	if (descriptor.pixelData.empty()) {
		Vector<Byte> blank;
		blank.resize(size.x * size.y * TextureDescriptor::getBitsPerPixel(descriptor.format));
		imageBytes = blank.data();
	} else {
		imageBytes = descriptor.pixelData.getBytes();
	}

	[metalTexture replaceRegion:region
		mipmapLevel:0
		withBytes:imageBytes
		bytesPerRow:bytesPerRow
	];

	MTLSamplerDescriptor* samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
	samplerDescriptor.maxAnisotropy = 1;
	auto filter = descriptor.useFiltering ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
	samplerDescriptor.minFilter = filter;
	samplerDescriptor.magFilter = filter;
	samplerDescriptor.mipFilter = descriptor.useFiltering ? MTLSamplerMipFilterLinear : MTLSamplerMipFilterNearest;
	auto addressMode = getMetalAddressMode(descriptor);
	samplerDescriptor.sAddressMode = addressMode;
	samplerDescriptor.rAddressMode = addressMode;
	samplerDescriptor.tAddressMode = addressMode;
	samplerDescriptor.lodMinClamp = 0;
	samplerDescriptor.lodMaxClamp = FLT_MAX;
	sampler = [video.getDevice() newSamplerStateWithDescriptor:samplerDescriptor];
	[samplerDescriptor release];

	doneLoading();
}

void MetalTexture::bind(id<MTLRenderCommandEncoder> encoder, int bindIndex) const
{
	waitForLoad();

	[encoder setFragmentTexture:metalTexture atIndex:bindIndex];
	[encoder setFragmentSamplerState:sampler atIndex:bindIndex];
}

MTLSamplerAddressMode MetalTexture::getMetalAddressMode(TextureDescriptor& descriptor)
{
	switch (descriptor.addressMode) {
	case TextureAddressMode::Clamp:
		return MTLSamplerAddressModeClampToEdge;
	case TextureAddressMode::Mirror:
		return MTLSamplerAddressModeMirrorRepeat;
	case TextureAddressMode::Repeat:
		return MTLSamplerAddressModeRepeat;
	}
}
