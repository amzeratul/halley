#pragma once

#include "../../core/src/graphics/render_target/render_target_texture.h"

namespace Halley
{
	class RenderTargetOpenGL : public TextureRenderTarget
	{
	public:
		~RenderTargetOpenGL();

		Rect4f getViewPort() const override;
		void bind() override;
		void unbind() override;
		std::unique_ptr<RenderTarget> makeSubArea(Rect4f area) override;

	private:
		void init();
		void deInit();

		GLuint fbo = 0;
	};
}
