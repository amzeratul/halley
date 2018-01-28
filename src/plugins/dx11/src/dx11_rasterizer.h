#pragma once
#include <d3d11.h>
#undef min
#undef max

namespace Halley
{
	class DX11Video;

	class DX11Rasterizer {
	public:
		DX11Rasterizer(DX11Video& video, bool enableScissor);
		~DX11Rasterizer();

		void bind(DX11Video& video);

	private:
		ID3D11RasterizerState* rasterizer = nullptr;
	};
}
