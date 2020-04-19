#pragma once
#include <halley/core/graphics/render_target/render_target_screen.h>
#include <halley/core/graphics/render_target/render_target_texture.h>
#include <Metal/Metal.h>
#include "metal_video.h"

namespace Halley {

	class IMetalRenderTarget
	{
	public:
		virtual ~IMetalRenderTarget() {}
		virtual id<MTLTexture> getMetalTexture() = 0;
	};

	class MetalScreenRenderTarget : public ScreenRenderTarget, public IMetalRenderTarget
	{
	public:
		explicit MetalScreenRenderTarget(MetalVideo& video, const Rect4i& viewPort);
		bool getViewportFlipVertical() const override;
		bool getProjectionFlipVertical() const override;
		void onBind(Painter& painter) override;
		void onUnbind(Painter& painter) override;

		id<MTLTexture> getMetalTexture() override;
	private:
		MetalVideo& video;
	};

	class MetalTextureRenderTarget : public TextureRenderTarget, public IMetalRenderTarget
	{
	public:
		bool getViewportFlipVertical() const override;
		bool getProjectionFlipVertical() const override;
		void onBind(Painter& painter) override;
		void onUnbind(Painter& painter) override;

		id<MTLTexture> getMetalTexture() override;
	};

}
