#pragma once

#include "halley/core/graphics/render_target/render_target_texture.h"
#include "halley_gl.h"
#include "halley/core/graphics/render_target/render_target_screen.h"

namespace Halley
{
	class TextureRenderTargetOpenGL final : public TextureRenderTarget
	{
	public:
		~TextureRenderTargetOpenGL();

		void onBind(Painter&) override;
		void onUnbind(Painter&) override;

	private:
		void init();
		void deInit();

		GLuint fbo = 0;
	};

	class ScreenRenderTargetOpenGL final : public ScreenRenderTarget
	{
	public:
		ScreenRenderTargetOpenGL(Rect4i rect) : ScreenRenderTarget(rect) {}

		bool getProjectionFlipVertical() const override;
		bool getViewportFlipVertical() const override;

		void onBind(Painter&) override;
	};
}
