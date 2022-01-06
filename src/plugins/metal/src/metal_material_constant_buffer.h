#pragma once
#include "metal_buffer.h"
#include <halley/core/graphics/material/material.h>

namespace Halley
{
	class MetalMaterialConstantBuffer : public MaterialConstantBuffer
	{
	public:
		explicit MetalMaterialConstantBuffer(MetalVideo& video);
		~MetalMaterialConstantBuffer();
		void update(gsl::span<const gsl::byte> data) override;
		void bindVertex(id<MTLRenderCommandEncoder> encoder, int bindPoint);
		void bindFragment(id<MTLRenderCommandEncoder> encoder, int bindPoint);
	private:
		MetalBuffer buffer;
	};
}
