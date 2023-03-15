#pragma once

#include "halley/graphics/render_target/render_target_texture.h"
#include "halley_gl.h"
#include "halley/graphics/render_target/render_target_screen.h"

namespace Halley
{
	class IRenderTargetOpenGL
	{
	public:
		virtual ~IRenderTargetOpenGL() {}

		virtual bool isScreenRenderTarget() const = 0;
	};

	class TextureRenderTargetOpenGL final : public TextureRenderTarget, public IRenderTargetOpenGL
	{
	public:
		~TextureRenderTargetOpenGL() override;

		bool isScreenRenderTarget() const override { return false; }

		void onBind(Painter&) override;
		void onUnbind(Painter&) override;

	private:
		void init();
		void deInit();

		GLuint fbo = 0;
	};

	class ScreenRenderTargetOpenGL final : public ScreenRenderTarget, public IRenderTargetOpenGL
	{
	public:
		ScreenRenderTargetOpenGL(Rect4i rect) : ScreenRenderTarget(rect) {}

		bool isScreenRenderTarget() const override { return true; }

		bool getProjectionFlipVertical() const override;
		bool getViewportFlipVertical() const override;

		void onBind(Painter&) override;
	};
}
