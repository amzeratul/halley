#pragma once
#include "halley/graphics/material/material.h"
#include "metal_buffer.h"

namespace Halley
{
	class MetalVideo;

	class MetalStructuredBuffer final : public MaterialStructuredBuffer
	{
	public:
		explicit MetalStructuredBuffer(MetalVideo& video);
		~MetalStructuredBuffer();

		void update(gsl::span<const std::byte> data, size_t elementStride) override;
		size_t getSize() const override { return dataSize; }
		size_t getStride() const override { return stride; }

		void bindVertex(id<MTLRenderCommandEncoder> encoder, int bindPoint);
		void bindFragment(id<MTLRenderCommandEncoder> encoder, int bindPoint);

	private:
		MetalBuffer buffer;
		size_t dataSize = 0;
		size_t stride = 0;
	};
}
