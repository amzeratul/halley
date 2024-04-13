#pragma once

#include "halley/api/halley_api_internal.h"
#include "halley/graphics/window.h"
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
		void waitForVsync() override;
		
		void setWindow(WindowDefinition&& windowDescriptor) override;
		Window& getWindow() const override;
		bool hasWindow() const override;
		void setVsync(bool vsync) override;
		bool hasVsync() const override;
		
		std::unique_ptr<Texture> createTexture(Vector2i size) override;
		std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) override;
		std::unique_ptr<TextureRenderTarget> createTextureRenderTarget() override;
		std::unique_ptr<ScreenRenderTarget> createScreenRenderTarget() override;
		std::unique_ptr<MaterialConstantBuffer> createConstantBuffer() override;
		std::unique_ptr<MaterialShaderStorageBuffer> createShaderStorageBuffer() override;

		void init() override;
		void deInit() override;
		void onResume() override;
		void onSuspend() override;

		std::unique_ptr<Painter> makePainter(Resources& resources) override;

		String getShaderLanguage() override;

		ID3D11Device& getDevice();
		ID3D11DeviceContext1& getDeviceContext();
		DX11SwapChain& getSwapChain();
		D3D_FEATURE_LEVEL getFeatureLevel() const;
		
		SystemAPI& getSystem();

		void* getImplementationPointer(const String& id) override;

	private:
		SystemAPI& system;
		std::shared_ptr<Window> window;

		ID3D11Device* device = nullptr;
		ID3D11DeviceContext1* deviceContext = nullptr;
		D3D_FEATURE_LEVEL featureLevel;

		bool initialised = false;
		bool useVsync = false;

		std::unique_ptr<DX11Loader> loader;
		std::unique_ptr<DX11SwapChain> swapChain;

		void initD3D(Window& window);
		void releaseD3D();
	};
}
