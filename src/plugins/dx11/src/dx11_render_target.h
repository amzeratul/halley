#pragma once
#include "halley/core/graphics/render_target/render_target_screen.h"
#include "halley/core/graphics/render_target/render_target_texture.h"

namespace Halley
{
	class DX11Video;

	class DX11ScreenRenderTarget : public ScreenRenderTarget
	{
	public:
		explicit DX11ScreenRenderTarget(DX11Video& video, const Rect4i& viewPort);

		bool getProjectionFlipVertical() const override;
		bool getViewportFlipVertical() const override;
		
		void onBind(Painter& painter) override;

	private:
		DX11Video& video;
	};

	class DX11TextureRenderTarget : public TextureRenderTarget
	{
	public:
		DX11TextureRenderTarget(DX11Video& video);

		void onBind(Painter& painter) override;

	private:
		DX11Video& video;
	};
}
