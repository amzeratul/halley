#pragma once

#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/texture_descriptor.h>

#include <Metal/Metal.h>

namespace Halley {
	class MetalVideo;

	class MetalTexture : public Texture
	{
	public:
		explicit MetalTexture(MetalVideo& video, Vector2i size);
		void load(TextureDescriptor&& descriptor) override;
		void bind(id<MTLRenderCommandEncoder> encoder, int bindIndex) const;

	private:
		MetalVideo& video;
		id<MTLTexture> metalTexture;
		id<MTLSamplerState> sampler;

		static MTLSamplerAddressMode getMetalAddressMode(TextureDescriptor& descriptor);
	};
}
