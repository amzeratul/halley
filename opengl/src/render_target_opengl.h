#pragma once

#include "../../core/src/graphics/render_target/render_target_texture.h"

namespace Halley
{
	class RenderTargetOpenGL : public TextureRenderTarget
	{
	public:
		Vector2f getSize() const override;
		Rect4f getViewPort() const override;
		void bind() override;
		void unbind() override;
	};
}
