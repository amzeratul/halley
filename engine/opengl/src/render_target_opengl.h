#pragma once

#include "halley/core/graphics/render_target/render_target_texture.h"
#include "halley_gl.h"

namespace Halley
{
	class RenderTargetOpenGL final : public TextureRenderTarget
	{
	public:
		~RenderTargetOpenGL();

		Rect4i getViewPort() const override;
		bool isScreen() const override { return false; }

		void bind() override;
		void unbind() override;

	private:
		void init();
		void deInit();

		GLuint fbo = 0;
	};
}
