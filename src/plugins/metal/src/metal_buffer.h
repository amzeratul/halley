#pragma once

#include <gsl/gsl>
#include <Metal/Metal.h>

namespace Halley {
	class MetalVideo;

	class MetalBuffer {
	public:
		enum class Type
		{
			Vertex,
			Index,
			Constant
		};

		MetalBuffer(MetalVideo& video, Type type, size_t initialSize = 0);
		MetalBuffer(MetalBuffer&& other) noexcept;
		~MetalBuffer();

		MetalBuffer(const MetalBuffer& other) = delete;

		MetalBuffer& operator=(const MetalBuffer& other) = delete;
		MetalBuffer& operator=(MetalBuffer&& other) = delete;

		void setData(gsl::span<const gsl::byte> data);
		void bind(id<MTLRenderCommandEncoder> encoder, int bindPoint);
		id<MTLBuffer> getBuffer();

	private:
		MetalVideo& video;
		Type type;
		id<MTLBuffer> buffer = nil;
	};
}
