#pragma once
#include <D3D11_1.h>
#undef min
#undef max

namespace Halley
{
	class MaterialDepthStencil;
	class DX11Video;

	class DX11DepthStencil
	{
	public:
		DX11DepthStencil(DX11Video& video, const MaterialDepthStencil& definition);
		~DX11DepthStencil();

	private:
		DX11Video& video;
		ID3D11DepthStencilState* state = nullptr;
	};
}
