#pragma once

#include <map>
#include <mutex>
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/graphics/window.h"

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
	};
}
