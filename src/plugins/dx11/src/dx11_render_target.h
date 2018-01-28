#pragma once
#include "halley/core/graphics/render_target/render_target_screen.h"
#include "halley/core/graphics/render_target/render_target_texture.h"

namespace Halley
{
	class DX11ScreenRenderTarget : public ScreenRenderTarget
	{
	public:
		explicit DX11ScreenRenderTarget(const Rect4i& viewPort);

		bool flipVertical() const override;
		void onStartDrawCall(Painter&) override;
	};

	class DX11TextureRenderTarget : public TextureRenderTarget
	{
	public:
		DX11TextureRenderTarget();
	};
}
