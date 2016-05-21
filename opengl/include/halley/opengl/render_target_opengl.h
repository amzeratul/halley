#pragma once

#include "halley/graphics/render_target/render_target_texture.h"
#include "halley_gl.h"

namespace Halley
{
	class RenderTargetOpenGL final : public TextureRenderTarget
	{
	public:
		~RenderTargetOpenGL();

		Rect4i getViewPort() const override;
		void bind() override;
		void unbind() override;

	private:
		void init();
		void deInit();

		GLuint fbo = 0;
	};
}
