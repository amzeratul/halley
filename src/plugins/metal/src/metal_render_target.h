#pragma once
#include <halley/graphics/render_target/render_target_screen.h>
#include <halley/graphics/render_target/render_target_texture.h>
#include <Metal/Metal.h>
#include "metal_video.h"
#include "metal_texture.h"

namespace Halley {

	class IMetalRenderTarget
	{
	public:
		virtual ~IMetalRenderTarget() {}
		virtual id<MTLTexture> getMetalTexture() = 0;
		virtual id<MTLTexture> getMetalDepthTexture() = 0;
		virtual int getScaleFactor() = 0;
	};

	class MetalScreenRenderTarget : public ScreenRenderTarget, public IMetalRenderTarget
	{
	public:
		explicit MetalScreenRenderTarget(MetalVideo& video, const Rect4i& viewPort, int scaleFactor);
		bool getViewportFlipVertical() const override;
		bool getProjectionFlipVertical() const override;
		void onBind(Painter& painter) override;
		void onUnbind(Painter& painter) override;

		id<MTLTexture> getMetalTexture() override;
		id<MTLTexture> getMetalDepthTexture() override;
		int getScaleFactor() override;
	private:
		MetalVideo& video;
		int scaleFactor;
		std::unique_ptr<MetalTexture> depthStencilBuffer;
	};

	class MetalTextureRenderTarget : public TextureRenderTarget, public IMetalRenderTarget
	{
	public:
		bool getViewportFlipVertical() const override;
		bool getProjectionFlipVertical() const override;
		void onBind(Painter& painter) override;
		void onUnbind(Painter& painter) override;

		id<MTLTexture> getMetalTexture() override;
		id<MTLTexture> getMetalDepthTexture() override;
		int getScaleFactor() override;
	};

}
