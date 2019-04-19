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
	auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
		width:descriptor.size.x
		height:descriptor.size.y
		mipmapped:descriptor.useMipMap
	];
	metalTexture = [video.getDevice() newTextureWithDescriptor:textureDescriptor];

	NSUInteger bytesPerRow = 4 * descriptor.size.x;
	MTLRegion region = {
		{ 0, 0, 0 },
		{static_cast<NSUInteger>(descriptor.size.x), static_cast<NSUInteger>(descriptor.size.y), 1}
	};

	[metalTexture replaceRegion:region
		mipmapLevel:0
		withBytes:descriptor.pixelData.getBytes()
		bytesPerRow:bytesPerRow
	];

	MTLSamplerDescriptor* samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
	samplerDescriptor.maxAnisotropy = 1;
	auto filter = descriptor.useFiltering ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
	samplerDescriptor.minFilter = filter;
	samplerDescriptor.magFilter = filter;
	samplerDescriptor.mipFilter = descriptor.useFiltering ? MTLSamplerMipFilterLinear : MTLSamplerMipFilterNearest;
	auto clamp = descriptor.clamp ? MTLSamplerAddressModeClampToEdge : MTLSamplerAddressModeRepeat;
	samplerDescriptor.sAddressMode = clamp;
	samplerDescriptor.rAddressMode = clamp;
	samplerDescriptor.tAddressMode = clamp;
	samplerDescriptor.lodMinClamp = 0;
	samplerDescriptor.lodMaxClamp = FLT_MAX;
	sampler = [video.getDevice() newSamplerStateWithDescriptor:samplerDescriptor];
	[samplerDescriptor release];

	doneLoading();
}

void MetalTexture::bind(id<MTLRenderCommandEncoder> encoder, int bindIndex) const {
	waitForLoad();

	[encoder setFragmentTexture:metalTexture atIndex:bindIndex];
	[encoder setFragmentSamplerState:sampler atIndex:bindIndex];
}

