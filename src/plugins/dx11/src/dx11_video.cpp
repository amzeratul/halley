#include "dx11_video.h"
#include "dx11_painter.h"
#include "dx11_shader.h"
#include "dx11_texture.h"
#include "dx11_render_target.h"
#include "dx11_material_constant_buffer.h"

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DXGI1_2.h>
#include <DXGI.h>
#include "dx11_loader.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "Dxgi.lib")

using namespace Halley;

DX11Video::DX11Video(SystemAPI& system)
	: system(system)
{}

void DX11Video::init()
{
	loader = std::make_unique<DX11Loader>(*this);
}

void DX11Video::deInit()
{
	loader.reset();
	releaseD3D();
}

void DX11Video::initD3D(Window& window, Rect4i view, bool vsync)
{
	if (initialised) {
		return;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));

	auto size = window.getWindowRect().getSize();

	swapChainDesc.Width = size.x;
	swapChainDesc.Height = size.y;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SampleDesc.Count = 1;

#ifdef WITH_UWP
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
#endif

	auto result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &deviceContext);
	if (result != S_OK) {
		throw Exception("Unable to initialise DX11");
	}

	IDXGIDevice2* pDXGIDevice;
	device->QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void **>(&pDXGIDevice));
	IDXGIAdapter * pDXGIAdapter;
	pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void **>(&pDXGIAdapter));
	IDXGIFactory2 * pIDXGIFactory;
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void **>(&pIDXGIFactory));

	if (window.getNativeHandleType() == "HWND") {
		auto hWnd = reinterpret_cast<HWND>(window.getNativeHandle());
		result = pIDXGIFactory->CreateSwapChainForHwnd(device, hWnd, &swapChainDesc, nullptr, nullptr, &swapChain);
		if (result != S_OK) {
			throw Exception("Unable to create swap chain for HWND");
		}
	} else if (window.getNativeHandleType() == "CoreWindow") {
		IUnknown* coreWindow = reinterpret_cast<IUnknown*>(window.getNativeHandle());
		result = pIDXGIFactory->CreateSwapChainForCoreWindow(device, coreWindow, &swapChainDesc, nullptr, &swapChain);
		if (result != S_OK) {
			throw Exception("Unable to create swap chain for CoreWindow");
		}
	}

	ID3D11Texture2D *pBackBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&pBackBuffer));
    device->CreateRenderTargetView(pBackBuffer, nullptr, &backbuffer);
    pBackBuffer->Release();
    deviceContext->OMSetRenderTargets(1, &backbuffer, nullptr);

	D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    viewport.TopLeftX = float(view.getLeft());
    viewport.TopLeftY = float(view.getTop());
    viewport.Width = float(view.getWidth());
    viewport.Height = float(view.getHeight());
    deviceContext->RSSetViewports(1, &viewport);

	initialised = true;
	useVsync = vsync;
}

void DX11Video::releaseD3D()
{
	if (!initialised) {
		return;
	}

	backbuffer->Release();
	swapChain->Release();
	device->Release();
	deviceContext->Release();
	initialised = false;
}

void DX11Video::startRender()
{
}

void DX11Video::finishRender()
{
    swapChain->Present(useVsync ? 1 : 0, 0);
}

void DX11Video::setWindow(WindowDefinition&& windowDescriptor, bool vsync)
{
	if (!window) {
		window = system.createWindow(windowDescriptor);

		initD3D(*window, Rect4i({}, window->getWindowRect().getSize()), vsync);
	} else {
		window->update(windowDescriptor);
	}
}

const Window& DX11Video::getWindow() const
{
	return *window;
}

bool DX11Video::hasWindow() const
{
	return !!window;
}

std::unique_ptr<Texture> DX11Video::createTexture(Vector2i size)
{
	return std::make_unique<DX11Texture>(*this, size);
}

std::unique_ptr<Shader> DX11Video::createShader(const ShaderDefinition& definition)
{
	return std::make_unique<DX11Shader>(*this, definition);
}

std::unique_ptr<TextureRenderTarget> DX11Video::createTextureRenderTarget()
{
	return std::make_unique<DX11TextureRenderTarget>(*this);
}

std::unique_ptr<ScreenRenderTarget> DX11Video::createScreenRenderTarget()
{
	return std::make_unique<DX11ScreenRenderTarget>(*this, Rect4i(Vector2i(), window->getWindowRect().getSize()), backbuffer);
}

std::unique_ptr<MaterialConstantBuffer> DX11Video::createConstantBuffer()
{
	return std::make_unique<DX11MaterialConstantBuffer>(*this);
}

std::unique_ptr<Painter> DX11Video::makePainter(Resources& resources)
{
	return std::make_unique<DX11Painter>(*this, resources);
}

String DX11Video::getShaderLanguage()
{
	return "hlsl";
}

ID3D11Device& DX11Video::getDevice()
{
	return *device;
}

ID3D11DeviceContext& DX11Video::getDeviceContext()
{
	return *deviceContext;
}

SystemAPI& DX11Video::getSystem()
{
	return system;
}
