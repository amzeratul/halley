#pragma once

#include <d3d11.h>
#include <D3D11_1.h>
#include <DXGI1_2.h>
#undef min
#undef max
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/graphics/window.h"

namespace Halley
{
	class Window;
	class DX11Video;

	class DX11SwapChain
	{
	public:
		DX11SwapChain(DX11Video& video, Window& window);
		~DX11SwapChain();
		
		Vector2i getSize() const;
		ID3D11RenderTargetView* getRenderTarget() const;

		void present(bool useVsync);
		void resize(Vector2i size);

	private:
		IDXGISwapChain1* swapChain = nullptr;
		ID3D11RenderTargetView* renderTarget = nullptr;

		Vector2i size;
		DX11Video& video;

		void init(Window& window);
		void initRenderTarget();
	};
}
