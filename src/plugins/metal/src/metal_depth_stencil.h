#pragma once
#include <Metal/Metal.h>
#undef min
#undef max
#include "halley/graphics/material/material_definition.h"

namespace Halley
{
	class MaterialDepthStencil;
	class MetalVideo;

	class MetalDepthStencil
	{
	public:
		MetalDepthStencil(MetalVideo& video, const MaterialDepthStencil& definition);
		~MetalDepthStencil();

		const MaterialDepthStencil& getDefinition() const;
		void bind(id<MTLRenderCommandEncoder> descriptor);

	private:
		MetalVideo& video;
		id<MTLDepthStencilState> state = nil;
		MaterialDepthStencil definition;
		int reference = 1;
	};
}
