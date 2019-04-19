#pragma once
#include <halley/core/graphics/render_target/render_target_screen.h>

namespace Halley {

	class MetalScreenRenderTarget : public ScreenRenderTarget
	{
	public:
		explicit MetalScreenRenderTarget(const Rect4i& viewPort);
		bool getViewportFlipVertical() const override;
		bool getProjectionFlipVertical() const override;
	};

}
