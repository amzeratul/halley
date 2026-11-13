#include "dx11_video.h"
#include "dx11_painter.h"
#include "dx11_shader.h"
#include "dx11_texture.h"
#include "dx11_render_target_screen.h"
#include "dx11_render_target_texture.h"
#include "dx11_material_constant_buffer.h"
#include "dx11_structured_buffer.h"

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DXGI1_2.h>
#include <DXGI.h>
#include <dxgi1_6.h>

#include "dx11_loader.h"
#include "halley/support/logger.h"
#include "halley/support/debug.h"
#include "dx11_swapchain.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "Dxgi.lib")

#ifdef DEV_BUILD
#pragma comment( lib, "dxguid.lib")
#endif

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

void DX11Video::onResume()
{
	loader = std::make_unique<DX11Loader>(*this);
}

void DX11Video::onSuspend()
{
	loader.reset();
}

namespace {

	IDXGIAdapter* getVideoAdapter()
	{
		IDXGIAdapter* adapter = nullptr;

		IDXGIFactory1* factory = nullptr;
		if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)) != S_OK) {
			return nullptr;
		}

		IDXGIFactory6* factory6 = nullptr;
		factory->QueryInterface(__uuidof(IDXGIFactory6), reinterpret_cast<void**>(&factory6));
		if (factory6) {
			IDXGIAdapter4* curAdapter = nullptr;
			Logger::logInfo("Available DX11 video adapters (DXGIFactory6):");
			for (int i = 0; ; ++i) {
				const auto result = factory6->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter4), reinterpret_cast<void**>(&curAdapter));
				if (result == S_OK) {
					DXGI_ADAPTER_DESC3 desc;
					curAdapter->GetDesc3(&desc);
					const bool choose = !adapter && (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) == 0;
					Logger::logInfo(String(choose ? "      * " : "        ") + toString(i) + ": " + String(desc.Description) + " (" + String::prettySize(desc.DedicatedVideoMemory) + ")" + (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE ? " [software]" : " [hardware]"));
					if (choose) {
						adapter = curAdapter;
					} else {
						curAdapter->Release();
					}
				} else {
					break;
				}
			}

			factory6->Release();
		} else {
			IDXGIAdapter* curAdapter = nullptr;
			Logger::logInfo("Available DX11 video adapters (DXGIFactory1):");
			for (int i = 0; ; ++i) {
				const auto result = factory->EnumAdapters(i, &curAdapter);
				if (result == S_OK) {
					DXGI_ADAPTER_DESC desc;
					curAdapter->GetDesc(&desc);
					Logger::logInfo(String(i == 0 ? "      * " : "        ") + toString(i) + ": " + String(desc.Description) + " (" + String::prettySize(desc.DedicatedVideoMemory) + ")");
					if (i == 0) {
						adapter = curAdapter;
					} else {
						curAdapter->Release();
					}
				} else {
					break;
				}
			}
		}

		factory->Release();

		return adapter;
	}
}

void DX11Video::initD3D(Window& window)
{
	if (initialised) {
		return;
	}

	auto* adapter = getVideoAdapter();

	ID3D11DeviceContext* dc;
	std::array<D3D_FEATURE_LEVEL, 3> featureLevels = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	uint32_t flags = 0;
	if (Debug::isDebug()) {
		flags |= D3D11_CREATE_DEVICE_DEBUG;
	}
	auto result = D3D11CreateDevice(adapter, adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevels.data(), static_cast<UINT>(featureLevels.size()), D3D11_SDK_VERSION, &device, &featureLevel, &dc);
	if (adapter) {
		adapter->Release();
	}
	if (result != S_OK) {
		throw Exception("Unable to initialise DX11, D3D11CreateDevice returned 0x" + toString(uint32_t(result), 16), HalleyExceptions::VideoPlugin);
	}
	result = dc->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&deviceContext));
	dc->Release();
	if (FAILED(result) || !deviceContext) {
		throw Exception("Unable to initialise DX11.1, QueryInterface returned 0x" + toString(uint32_t(result), 16), HalleyExceptions::VideoPlugin);
	}

	ID3D10Multithread* mt = nullptr;
	device->QueryInterface(__uuidof(ID3D10Multithread), reinterpret_cast<void**>(&mt));
	if (mt) {
		mt->SetMultithreadProtected(true);
		mt->Release();
	}

	swapChain = std::make_unique<DX11SwapChain>(*this, window, 3);

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

void DX11Video::waitForVsync()
{
	swapChain->waitForVsync();
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

Window& DX11Video::getWindow() const
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

bool DX11Video::hasVsync() const
{
	return useVsync;
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

std::unique_ptr<MaterialStructuredBuffer> DX11Video::createStructuredBuffer()
{
	return std::make_unique<DX11StructuredBuffer>(*this);
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
	HalleyAssertDev(device != nullptr);
	return *device;
}

ID3D11DeviceContext1& DX11Video::getDeviceContext()
{
	HalleyAssertDev(deviceContext);
	return *deviceContext;
}

DX11SwapChain& DX11Video::getSwapChain()
{
	HalleyAssertDev(swapChain != nullptr);
	return *swapChain;
}

D3D_FEATURE_LEVEL DX11Video::getFeatureLevel() const
{
	return featureLevel;
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

bool DX11Video::needsVideoAux() const
{
	return true;
}
