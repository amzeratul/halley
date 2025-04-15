#include "metal_render_target.h"
#include "metal_painter.h"
#include "metal_texture.h"
#include <memory>

using namespace Halley;

MetalScreenRenderTarget::MetalScreenRenderTarget(MetalVideo& video, const Rect4i& viewPort, int scaleFactor)
	: ScreenRenderTarget(viewPort)
	, video(video)
	, scaleFactor(scaleFactor)
{
	depthStencilBuffer = std::make_unique<MetalTexture>(video, viewPort.getSize() );
	TextureDescriptor descriptor{ viewPort.getSize(), TextureFormat::Depth };
	descriptor.isDepthStencil = true;
	depthStencilBuffer->load( std::move( descriptor ));
}

bool MetalScreenRenderTarget::getViewportFlipVertical() const {
	return false;
}

bool MetalScreenRenderTarget::getProjectionFlipVertical() const {
	return true;
}

void MetalScreenRenderTarget::onBind(Painter& painter) {
	dynamic_cast<MetalPainter&>(painter).startEncoding( this );
}

void MetalScreenRenderTarget::onUnbind(Painter& painter) {
	dynamic_cast<MetalPainter&>(painter).endEncoding();
}

id<MTLTexture> MetalScreenRenderTarget::getMetalTexture() {
	return video.getSurface().texture;
}

id<MTLTexture> MetalScreenRenderTarget::getMetalDepthTexture() {
	if (depthStencilBuffer) {
		return depthStencilBuffer->metalTexture;
	}

	return nil;
}

int MetalScreenRenderTarget::getScaleFactor() {
	return scaleFactor;
}

bool MetalTextureRenderTarget::getViewportFlipVertical() const {
	return false;
}

bool MetalTextureRenderTarget::getProjectionFlipVertical() const {
	return true;
}

void MetalTextureRenderTarget::onBind(Painter& painter) {
	dynamic_cast<MetalPainter&>(painter).startEncoding(this);
}

void MetalTextureRenderTarget::onUnbind(Painter& painter) {
	dynamic_cast<MetalPainter&>(painter).endEncoding();
}

id<MTLTexture> MetalTextureRenderTarget::getMetalTexture() {
	return std::static_pointer_cast<MetalTexture>(getTexture(0))->metalTexture;
}

id<MTLTexture> MetalTextureRenderTarget::getMetalDepthTexture() {
	auto texture = std::static_pointer_cast<MetalTexture>(getDepthTexture());
	return texture ? texture->metalTexture : nil;
}

int MetalTextureRenderTarget::getScaleFactor() {
	return 1;
}
