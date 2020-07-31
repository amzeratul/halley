#pragma once
#include <D3D11_1.h>
#undef min
#undef max
#include "halley/core/graphics/material/material_definition.h"

namespace Halley
{
	class MaterialDepthStencil;
	class DX11Video;

	class DX11DepthStencil
	{
	public:
		DX11DepthStencil(DX11Video& video, const MaterialDepthStencil& definition);
		~DX11DepthStencil();

		const MaterialDepthStencil& getDefinition() const;
		void bind();

	private:
		DX11Video& video;
		ID3D11DepthStencilState* state = nullptr;
		MaterialDepthStencil definition;
		int reference = 1;
	};
}
