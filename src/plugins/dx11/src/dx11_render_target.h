#pragma once
#include <d3d11.h>

namespace Halley
{
	class DX11Video;

	class IDX11RenderTarget
	{
	public:
		virtual ~IDX11RenderTarget() {}

		virtual ID3D11RenderTargetView* getRenderTargetView() = 0;
		virtual ID3D11DepthStencilView* getDepthStencilView() = 0;
	};
}
