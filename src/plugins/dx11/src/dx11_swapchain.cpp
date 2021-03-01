#include "dx11_swapchain.h"
#include "halley/core/graphics/window.h"
#include "dx11_video.h"
#include "halley/core/graphics/texture.h"
#include "dx11_texture.h"
#include "halley/core/graphics/texture_descriptor.h"
using namespace Halley;

constexpr static int numBuffers = 2;

DX11SwapChain::DX11SwapChain(DX11Video& video, Window& window)
	: video(video)
{
	init(window);
}

DX11SwapChain::~DX11SwapChain()
{
	clearRenderTarget();
	clearDepthStencilViews();

	if (swapChain) {
		swapChain->Release();
		swapChain = nullptr;
	}
}

void DX11SwapChain::present(bool useVsync)
{
	swapChain->Present(useVsync ? 1 : 0, 0);
	curBuffer = (curBuffer + 1) % numBuffers;
}

void DX11SwapChain::resize(Vector2i newSize)
{
	clearRenderTarget();
	
	HRESULT result = swapChain->ResizeBuffers(0, newSize.x, newSize.y, DXGI_FORMAT_UNKNOWN, 0);
	if (result != S_OK) {
		throw Exception("Unable to resize swap chain", HalleyExceptions::VideoPlugin);
	}

	size = newSize;

	initRenderTarget();
}

Vector2i DX11SwapChain::getSize() const
{
	return size;
}

ID3D11RenderTargetView* DX11SwapChain::getRenderTargetView() const
{
	return renderTarget;
}

ID3D11DepthStencilView* DX11SwapChain::getDepthStencilView() const
{
	if (depthStencilViews.empty()) {
		return nullptr;
	}
	return depthStencilViews.at(curBuffer);
}

void DX11SwapChain::init(Window& window)
{
	if (swapChain) {
		swapChain->Release();
		swapChain = nullptr;
	}

	size = window.getWindowRect().getSize();
	const bool usingCoreWindow = window.getNativeHandleType() == "CoreWindow";

	IDXGIDevice2* pDXGIDevice;
	video.getDevice().QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void **>(&pDXGIDevice));
	IDXGIAdapter * pDXGIAdapter;
	pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void **>(&pDXGIAdapter));
	IDXGIFactory2 * pIDXGIFactory;
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void **>(&pIDXGIFactory));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	swapChainDesc.Width = size.x;
	swapChainDesc.Height = size.y;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SampleDesc.Count = 1;

	if (usingCoreWindow) {
#if WINVER >= 0x0A00
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#else
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
#endif
		swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		IUnknown* coreWindow = reinterpret_cast<IUnknown*>(window.getNativeHandle());
		auto result = pIDXGIFactory->CreateSwapChainForCoreWindow(&video.getDevice(), coreWindow, &swapChainDesc, nullptr, &swapChain);
		if (result != S_OK) {
			throw Exception("Unable to create swap chain for CoreWindow", HalleyExceptions::VideoPlugin);
		}
	} else {
		auto hWnd = reinterpret_cast<HWND>(window.getNativeHandle());
		auto result = pIDXGIFactory->CreateSwapChainForHwnd(&video.getDevice(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain);
		if (result != S_OK) {
			throw Exception("Unable to create swap chain for HWND", HalleyExceptions::VideoPlugin);
		}
	}

	initRenderTarget();
}

void DX11SwapChain::initRenderTarget()
{
	clearRenderTarget();

	ID3D11Texture2D *pBackBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&pBackBuffer));
    video.getDevice().CreateRenderTargetView(pBackBuffer, nullptr, &renderTarget);
    pBackBuffer->Release();

	initDepthStencilViews();
}

void DX11SwapChain::initDepthStencilViews()
{
	clearDepthStencilViews();

	for (int i = 0; i < numBuffers; ++i) {
		auto t = video.createTexture(size);
		auto& tex = dynamic_cast<DX11Texture&>(*t);
		TextureDescriptor descriptor;
		descriptor.size = size;
		descriptor.isDepthStencil = true;
		descriptor.format = TextureFormat::Depth;
		tex.startLoading();
		tex.load(std::move(descriptor));

		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		desc.Flags = 0;
		desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;
		ID3D11DepthStencilView *view;
		auto result = video.getDevice().CreateDepthStencilView(tex.getTexture(), &desc, &view);
		if (result != S_OK) {
			throw Exception("Unable to create DepthStencilView", HalleyExceptions::VideoPlugin);
		}

		depthStencilTextures.push_back(std::move(t));
		depthStencilViews.push_back(view);
	}
}

void DX11SwapChain::clearRenderTarget()
{
	if (renderTarget) {
		renderTarget->Release();
		renderTarget = nullptr;
	}
}

void DX11SwapChain::clearDepthStencilViews()
{
	for (auto& view: depthStencilViews) {
		view->Release();
	}
	depthStencilViews.clear();
	depthStencilTextures.clear();
}
