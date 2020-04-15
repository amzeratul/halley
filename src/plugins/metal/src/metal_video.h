#pragma once
#include "metal_loader.h"
#include "metal_shader.h"
#include <halley/core/api/halley_api_internal.h>
#include <halley/core/graphics/painter.h>
#include <halley/core/graphics/material/material_definition.h>
#include <halley/core/graphics/render_target/render_target_texture.h>
#include <halley/core/graphics/render_target/render_target_screen.h>
#include <QuartzCore/CAMetalLayer.h>

namespace Halley {
	// Place vertex buffer at end of table to avoid collisions.
	static const size_t MaxMetalBufferIndex = 30;

	class MetalVideo final : public VideoAPIInternal
	{
	public:
		explicit MetalVideo(SystemAPI& system);

		void startRender() override;
		void finishRender() override;
		void setWindow(WindowDefinition&& windowDescriptor) override;
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
		String getShaderLanguage() override;
		bool isColumnMajor() const override;

		id<CAMetalDrawable> getSurface();
		id<MTLCommandQueue> getCommandQueue();
		id<MTLDevice> getDevice();

	private:
		std::shared_ptr<Window> window;
		std::unique_ptr<MetalLoader> loader;
		SystemAPI& system;
		CAMetalLayer* swap_chain;
		id<CAMetalDrawable> surface;
		id<MTLDevice> device;
		id<MTLCommandQueue> command_queue;
		NSAutoreleasePool *pool;

		void initSwapChain(Window& window);
	};

}
