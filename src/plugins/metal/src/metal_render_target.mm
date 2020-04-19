#include "metal_render_target.h"
#include "metal_painter.h"
#include "metal_texture.h"
#include <memory>

using namespace Halley;

MetalScreenRenderTarget::MetalScreenRenderTarget(MetalVideo& video, const Rect4i& viewPort)
	: ScreenRenderTarget(viewPort)
	, video(video)
{}

bool MetalScreenRenderTarget::getViewportFlipVertical() const {
	return false;
}

bool MetalScreenRenderTarget::getProjectionFlipVertical() const {
	return true;
}

void MetalScreenRenderTarget::onBind(Painter& painter) {
	dynamic_cast<MetalPainter&>(painter).startEncoding(getMetalTexture());
}

void MetalScreenRenderTarget::onUnbind(Painter& painter) {
	dynamic_cast<MetalPainter&>(painter).endEncoding();
}

id<MTLTexture> MetalScreenRenderTarget::getMetalTexture() {
	return video.getSurface().texture;
};

bool MetalTextureRenderTarget::getViewportFlipVertical() const {
	return false;
}

bool MetalTextureRenderTarget::getProjectionFlipVertical() const {
	return true;
}

void MetalTextureRenderTarget::onBind(Painter& painter) {
	dynamic_cast<MetalPainter&>(painter).startEncoding(getMetalTexture());
}

void MetalTextureRenderTarget::onUnbind(Painter& painter) {
	dynamic_cast<MetalPainter&>(painter).endEncoding();
}

id<MTLTexture> MetalTextureRenderTarget::getMetalTexture() {
	return std::static_pointer_cast<MetalTexture>(getTexture(0))->metalTexture;
};
