#pragma once

#include <d3d11.h>
#include <D3D11_1.h>
#include <DXGI1_2.h>
#include "halley/api/halley_api_internal.h"
#include "halley/graphics/window.h"

namespace Halley
{
	class Window;
	class DX11Video;

	class DX11SwapChain
	{
	public:
		DX11SwapChain(DX11Video& video, Window& window, int numBuffers);
		~DX11SwapChain();
		
		Vector2i getSize() const;
		ID3D11RenderTargetView* getRenderTargetView() const;
		ID3D11DepthStencilView* getDepthStencilView() const;

		void present(bool useVsync);
		void waitForVsync();
		void resize(Vector2i size);

	private:
		IDXGISwapChain1* swapChain = nullptr;
		ID3D11RenderTargetView* renderTarget = nullptr;
		ID3D11DepthStencilView* depthStencilView;
		std::unique_ptr<Texture> depthStencilTexture;

		Vector2i size;
		DX11Video& video;
		int numBuffers = 2;

		void init(Window& window);
		void initRenderTarget();
		void initDepthStencilView();

		void clearRenderTarget();
		void clearDepthStencilView();
	};
}
