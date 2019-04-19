#include "metal_render_target.h"

using namespace Halley;

MetalScreenRenderTarget::MetalScreenRenderTarget(const Rect4i& viewPort)
	: ScreenRenderTarget(viewPort)
{}

bool MetalScreenRenderTarget::getViewportFlipVertical() const {
	return false;
}

bool MetalScreenRenderTarget::getProjectionFlipVertical() const {
	return true;
}
