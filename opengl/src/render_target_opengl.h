#pragma once

#include "../../core/src/graphics/render_target/render_target_texture.h"

namespace Halley
{
	class RenderTargetOpenGL : public TextureRenderTarget
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
