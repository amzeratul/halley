#pragma once

#include <d3d11.h>
#include <D3D11_1.h>
#include <DXGI1_2.h>
#include <dxgi1_3.h>
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
		IDXGISwapChain1* swapChain1 = nullptr;
		HANDLE frameLatencyWaitableObject = nullptr;
		uint32_t flags = 0;

		ID3D11RenderTargetView* renderTarget = nullptr;
		ID3D11DepthStencilView* depthStencilView;
		std::unique_ptr<Texture> depthStencilTexture;

		Vector2i size;
		DX11Video& video;
		int numBuffers = 2;

		bool waitedSincePresent = false;

		void init(Window& window);
		void deInit();
		void initRenderTarget();
		void initDepthStencilView();

		void clearRenderTarget();
		void clearDepthStencilView();
	};
}
