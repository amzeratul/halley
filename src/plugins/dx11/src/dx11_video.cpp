#include "dx11_video.h"
#include "dx11_painter.h"
#include "dx11_shader.h"
#include "dx11_texture.h"
#include "dx11_render_target_screen.h"
#include "dx11_render_target_texture.h"
#include "dx11_material_constant_buffer.h"

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DXGI1_2.h>
#include <DXGI.h>
#include "dx11_loader.h"
#include "halley/support/logger.h"
#include "halley/support/debug.h"
#include "dx11_swapchain.h"

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

#ifndef WINDOWS_STORE
	{
		IDXGIFactory* factory;
		CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));
		IDXGIAdapter* adapter;
		factory->EnumAdapters(0, &adapter);
		if (adapter) {
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);
			Logger::logInfo("Using display adapter for DX11: " + String(desc.Description));
		}
	}
#endif

	ID3D11DeviceContext* dc;
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	uint32_t flags = 0;
	if (Debug::isDebug()) {
		flags |= D3D11_CREATE_DEVICE_DEBUG;
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

	swapChain = std::make_unique<DX11SwapChain>(*this, window);

	initialised = true;
}

void DX11Video::releaseD3D()
{
	if (!initialised) {
		return;
	}

	swapChain.reset();

	device->Release();
	deviceContext->Release();
	initialised = false;
}

void DX11Video::startRender()
{
}

void DX11Video::finishRender()
{
	swapChain->present(useVsync);
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
	if (swapChain->getSize() != view.getSize()) {
		swapChain->resize(view.getSize());
	}

	return std::make_unique<DX11ScreenRenderTarget>(*this, view);
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
	Expects(device != nullptr);
	return *device;
}

ID3D11DeviceContext1& DX11Video::getDeviceContext()
{
	Expects(deviceContext);
	return *deviceContext;
}

DX11SwapChain& DX11Video::getSwapChain()
{
	Expects(swapChain != nullptr);
	return *swapChain;
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
