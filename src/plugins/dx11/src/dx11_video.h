#pragma once

#include "halley/core/api/halley_api_internal.h"
#include "halley/core/graphics/window.h"
#include <d3d11.h>
#include <D3D11_1.h>
#include <DXGI1_2.h>
#undef min
#undef max

namespace Halley {
	class SystemAPI;
	class DX11Loader;
	class DX11SwapChain;

	class DX11Video final : public VideoAPIInternal
	{
	public:
		DX11Video(SystemAPI& system);

		void startRender() override;
		void finishRender() override;
		
		void setWindow(WindowDefinition&& windowDescriptor) override;
		const Window& getWindow() const override;
		bool hasWindow() const override;
		void setVsync(bool vsync) override;
		
		std::unique_ptr<Texture> createTexture(Vector2i size) override;
		std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) override;
		std::unique_ptr<TextureRenderTarget> createTextureRenderTarget() override;
		std::unique_ptr<ScreenRenderTarget> createScreenRenderTarget() override;
		std::unique_ptr<MaterialConstantBuffer> createConstantBuffer() override;
		
		void init() override;
		void deInit() override;
		
		std::unique_ptr<Painter> makePainter(Resources& resources) override;

		String getShaderLanguage() override;

		ID3D11Device& getDevice();
		ID3D11DeviceContext1& getDeviceContext();
		
		SystemAPI& getSystem();

		void* getImplementationPointer(const String& id) override;

	private:
		SystemAPI& system;
		std::shared_ptr<Window> window;

		ID3D11Device* device = nullptr;
		ID3D11DeviceContext1* deviceContext = nullptr;

		bool initialised = false;
		bool useVsync = false;

		std::unique_ptr<DX11Loader> loader;
		std::unique_ptr<DX11SwapChain> swapChain;

		void initD3D(Window& window);
		void releaseD3D();
	};
}
