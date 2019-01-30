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
#include "halley/support/logger.h"
#include "halley/support/debug.h"

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

void DX11Video::initD3D(Window& window)
{
	if (initialised) {
		return;
	}

	IDXGIFactory* factory;
	CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));
	IDXGIAdapter* adapter;
	factory->EnumAdapters(0, &adapter);
	if (adapter) {
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);
		Logger::logInfo("Using display adapter for DX11: " + String(desc.Description));
	}

	ID3D11DeviceContext* dc;
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	uint32_t flags = 0;
	if (Debug::isDebug()) {
		flags |= D3D11_CREATE_DEVICE_DEBUG;
	} else {
		flags |= D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
	}
	auto result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevels, 3, D3D11_SDK_VERSION, &device, nullptr, &dc);
	if (result != S_OK) {
		throw Exception("Unable to initialise DX11", HalleyExceptions::VideoPlugin);
	}
	dc->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&deviceContext));
	if (!dc) {
		throw Exception("Unable to initialise DX11.1", HalleyExceptions::VideoPlugin);
	}

	ID3D10Multithread* mt;
	device->QueryInterface(__uuidof(ID3D10Multithread), reinterpret_cast<void**>(&mt));
	if (mt) {
		mt->SetMultithreadProtected(true);
	}

	initSwapChain(window);

	initialised = true;
}

void DX11Video::initSwapChain(Window& window)
{
	if (swapChain) {
		swapChain->Release();
		swapChain = nullptr;
	}

	auto size = window.getWindowRect().getSize();
	const bool usingCoreWindow = window.getNativeHandleType() == "CoreWindow";

	IDXGIDevice2* pDXGIDevice;
	device->QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void **>(&pDXGIDevice));
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
		auto result = pIDXGIFactory->CreateSwapChainForCoreWindow(device, coreWindow, &swapChainDesc, nullptr, &swapChain);
		if (result != S_OK) {
			throw Exception("Unable to create swap chain for CoreWindow", HalleyExceptions::VideoPlugin);
		}
	} else {
		auto hWnd = reinterpret_cast<HWND>(window.getNativeHandle());
		auto result = pIDXGIFactory->CreateSwapChainForHwnd(device, hWnd, &swapChainDesc, nullptr, nullptr, &swapChain);
		if (result != S_OK) {
			throw Exception("Unable to create swap chain for HWND", HalleyExceptions::VideoPlugin);
		}
	}

	swapChainSize = size;

	initBackBuffer();
}

void DX11Video::initBackBuffer()
{
	if (backbuffer) {
		backbuffer->Release();
		backbuffer = nullptr;
	}

	ID3D11Texture2D *pBackBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&pBackBuffer));
    device->CreateRenderTargetView(pBackBuffer, nullptr, &backbuffer);
    pBackBuffer->Release();
}

void DX11Video::resizeSwapChain(Vector2i size)
{
	if (backbuffer) {
		backbuffer->Release();
		backbuffer = nullptr;
	}
	
	HRESULT result = swapChain->ResizeBuffers(0, size.x, size.y, DXGI_FORMAT_UNKNOWN, 0);
	if (result != S_OK) {
		throw Exception("Unable to resize swap chain", HalleyExceptions::VideoPlugin);
	}

	swapChainSize = size;

	initBackBuffer();
}

void DX11Video::releaseD3D()
{
	if (!initialised) {
		return;
	}

	if (backbuffer) {
		backbuffer->Release();
		backbuffer = nullptr;
	}

	if (swapChain) {
		swapChain->Release();
		swapChain = nullptr;
	}

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

void DX11Video::setWindow(WindowDefinition&& windowDescriptor)
{
	if (!window) {
		window = system.createWindow(windowDescriptor);
		initD3D(*window);
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

void DX11Video::setVsync(bool vsync)
{
	useVsync = vsync;
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
	auto view = Rect4i(Vector2i(), window->getWindowRect().getSize());
	if (swapChainSize != view.getSize()) {
		resizeSwapChain(view.getSize());
	}

	return std::make_unique<DX11ScreenRenderTarget>(*this, view, backbuffer);
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

ID3D11DeviceContext1& DX11Video::getDeviceContext()
{
	return *deviceContext;
}

SystemAPI& DX11Video::getSystem()
{
	return system;
}

void* DX11Video::getImplementationPointer(const String& id)
{
	if (id == "ID3D11Device") {
		return static_cast<IUnknown*>(device);
	}
	return nullptr;
}
