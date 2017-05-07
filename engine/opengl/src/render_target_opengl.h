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

		void bind() override;
		void unbind() override;

	private:
		void init();
		void deInit();

		GLuint fbo = 0;
	};

	class ScreenRenderTargetOpenGL final : public ScreenRenderTarget
	{
	public:
		ScreenRenderTargetOpenGL(Rect4i rect) : ScreenRenderTarget(rect) {}

		void bind() override;
		void unbind() override;
	};
}
