#pragma once

#include <map>
#include <mutex>
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/graphics/window.h"
#include <d3d11.h>
#include <DXGI1_2.h>
#undef min
#undef max

namespace Halley {
	class SystemAPI;

	class DX11Video final : public VideoAPIInternal
	{
	public:
		DX11Video(SystemAPI& system);

		void startRender() override;
		void finishRender() override;
		
		void setWindow(WindowDefinition&& windowDescriptor, bool vsync) override;
		const Window& getWindow() const override;
		bool hasWindow() const override;
		
		std::unique_ptr<Texture> createTexture(Vector2i size) override;
		std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) override;
		std::unique_ptr<TextureRenderTarget> createTextureRenderTarget() override;
		std::unique_ptr<ScreenRenderTarget> createScreenRenderTarget() override;
		std::unique_ptr<MaterialConstantBuffer> createConstantBuffer() override;
		
		void init() override;
		void deInit() override;
		
		std::unique_ptr<Painter> makePainter(Resources& resources) override;

	private:
		SystemAPI& system;
		std::shared_ptr<Window> window;

		bool initialised = false;
		IDXGISwapChain1 *swapChain;
		ID3D11Device *device;
		ID3D11DeviceContext *deviceContext;
		ID3D11RenderTargetView *backbuffer;

		void initD3D(HWND hWnd, Rect4i view);
		void releaseD3D();
	};
}
