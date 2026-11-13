#include "dx11_swapchain.h"
#include "halley/graphics/window.h"
#include "dx11_video.h"
#include "halley/graphics/texture.h"
#include "dx11_texture.h"
#include "halley/graphics/texture_descriptor.h"
using namespace Halley;

DX11SwapChain::DX11SwapChain(DX11Video& video, Window& window, int numBuffers)
	: video(video)
	, numBuffers(numBuffers)
{
	init(window);
}

DX11SwapChain::~DX11SwapChain()
{
	clearRenderTarget();
	clearDepthStencilView();

	deInit();
}

void DX11SwapChain::present(bool useVsync)
{
	swapChain1->Present(useVsync ? 1 : 0, 0);
	waitedSincePresent = false;
}

void DX11SwapChain::waitForVsync()
{
	if (frameLatencyWaitableObject && !waitedSincePresent) {
		waitedSincePresent = true;
		WaitForSingleObjectEx(frameLatencyWaitableObject, 1000, TRUE);
	} else {
		// Already waited this frame, or no waitable object (< Win10)
		IDXGIOutput* output = nullptr;
		if (swapChain1->GetContainingOutput(&output) == S_OK) {
			output->WaitForVBlank();
			output->Release();
		} else {
			// Fallback
			Sleep(1);
		}
	}
}

void DX11SwapChain::resize(Vector2i newSize)
{
	clearRenderTarget();
	
	HRESULT result = swapChain1->ResizeBuffers(0, newSize.x, newSize.y, DXGI_FORMAT_UNKNOWN, flags);
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
	return depthStencilView;
}

void DX11SwapChain::init(Window& window)
{
	deInit();

	size = window.getWindowRect().getSize();
	const bool usingCoreWindow = window.getNativeHandleType() == "CoreWindow";

	IDXGIFactory2* pIDXGIFactory = nullptr;
	{
		IDXGIDevice2* pDXGIDevice;
		IDXGIAdapter * pDXGIAdapter;

		video.getDevice().QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void **>(&pDXGIDevice));
		pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void **>(&pDXGIAdapter));
		pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void **>(&pIDXGIFactory));

		pDXGIAdapter->Release();
		pDXGIDevice->Release();
	}

	flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	swapChainDesc.Width = size.x;
	swapChainDesc.Height = size.y;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = numBuffers;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = flags;

	if (usingCoreWindow) {
		swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		IUnknown* coreWindow = reinterpret_cast<IUnknown*>(window.getNativeHandle());
		auto result = pIDXGIFactory->CreateSwapChainForCoreWindow(&video.getDevice(), coreWindow, &swapChainDesc, nullptr, &swapChain1);
		if (result != S_OK) {
			pIDXGIFactory->Release();
			throw Exception("Unable to create swap chain for CoreWindow", HalleyExceptions::VideoPlugin);
		}
	} else {
		auto hWnd = reinterpret_cast<HWND>(window.getNativeHandle());
		auto result = pIDXGIFactory->CreateSwapChainForHwnd(&video.getDevice(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
		if (result != S_OK) {
			pIDXGIFactory->Release();
			throw Exception("Unable to create swap chain for HWND", HalleyExceptions::VideoPlugin);
		}

		pIDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_WINDOW_CHANGES);
	}
	pIDXGIFactory->Release();

	IDXGISwapChain2* swapChain2 = nullptr;
	swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain2));
	if (swapChain2) {
		// Win10+ should always have this
		swapChain2->SetMaximumFrameLatency(1); // Could also be numBuffers - 1?
		frameLatencyWaitableObject = swapChain2->GetFrameLatencyWaitableObject();
		swapChain2->Release();
	}

	initRenderTarget();
}

void DX11SwapChain::deInit()
{
	if (swapChain1) {
		swapChain1->Release();
		swapChain1 = nullptr;
	}
	if (frameLatencyWaitableObject) {
		CloseHandle(frameLatencyWaitableObject);
		frameLatencyWaitableObject = nullptr;
	}
}

void DX11SwapChain::initRenderTarget()
{
	clearRenderTarget();

	ID3D11Texture2D *pBackBuffer;
    swapChain1->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&pBackBuffer));
    video.getDevice().CreateRenderTargetView(pBackBuffer, nullptr, &renderTarget);
    pBackBuffer->Release();

	initDepthStencilView();
}

void DX11SwapChain::initDepthStencilView()
{
	clearDepthStencilView();

	depthStencilTexture = video.createTexture(size);
	auto& tex = dynamic_cast<DX11Texture&>(*depthStencilTexture);
	TextureDescriptor descriptor;
	descriptor.size = size;
	descriptor.isDepthStencil = true;
	descriptor.format = TextureFormat::Depth;
	tex.load(std::move(descriptor));

	D3D11_DEPTH_STENCIL_VIEW_DESC desc;
	desc.Flags = 0;
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;
	auto result = video.getDevice().CreateDepthStencilView(tex.getTexture(), &desc, &depthStencilView);
	if (result != S_OK) {
		throw Exception("Unable to create DepthStencilView", HalleyExceptions::VideoPlugin);
	}
}

void DX11SwapChain::clearRenderTarget()
{
	if (renderTarget) {
		renderTarget->Release();
		renderTarget = nullptr;
	}
}

void DX11SwapChain::clearDepthStencilView()
{
	if (depthStencilView) {
		depthStencilView->Release();
		depthStencilView = nullptr;
	}
	depthStencilTexture = {};
}
