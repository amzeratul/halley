#include "dx11_video.h"
#include "dx11_painter.h"
#include "dx11_shader.h"
#include "dx11_texture.h"
#include "dx11_render_target.h"
#include "dx11_material_constant_buffer.h"

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>

#pragma comment (lib, "d3d11.lib")

using namespace Halley;

DX11Video::DX11Video(SystemAPI& system)
	: system(system)
{}

void DX11Video::init()
{
}

void DX11Video::deInit()
{
	releaseD3D();
}

void DX11Video::initD3D(HWND hWnd, Rect4i view)
{
	if (initialised) {
		return;
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = true;

	HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, nullptr, &deviceContext);
	if (result != S_OK) {
		throw Exception("Unable to initialise DX11");
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
	float colour[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	deviceContext->ClearRenderTargetView(backbuffer, colour);
    swapChain->Present(0, 0);
}

void DX11Video::finishRender() {}

void DX11Video::setWindow(WindowDefinition&& windowDescriptor, bool vsync)
{
	if (!window) {
		window = system.createWindow(windowDescriptor);

		initD3D(reinterpret_cast<HWND>(window->getNativeHandle()), Rect4i({}, window->getWindowRect().getSize()));
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
	return false;
}

std::unique_ptr<Texture> DX11Video::createTexture(Vector2i size)
{
	return std::make_unique<DX11Texture>(size);
}

std::unique_ptr<Shader> DX11Video::createShader(const ShaderDefinition& definition)
{
	return std::make_unique<DX11Shader>(definition);
}

std::unique_ptr<TextureRenderTarget> DX11Video::createTextureRenderTarget()
{
	return std::make_unique<DX11TextureRenderTarget>();
}

std::unique_ptr<ScreenRenderTarget> DX11Video::createScreenRenderTarget()
{
	return std::make_unique<DX11ScreenRenderTarget>(Rect4i(Vector2i(), window->getWindowRect().getSize()));
}

std::unique_ptr<MaterialConstantBuffer> DX11Video::createConstantBuffer()
{
	return std::make_unique<DX11MaterialConstantBuffer>();
}

std::unique_ptr<Painter> DX11Video::makePainter(Resources& resources)
{
	return std::make_unique<DX11Painter>(resources);
}
