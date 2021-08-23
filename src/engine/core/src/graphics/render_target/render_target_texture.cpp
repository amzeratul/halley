#include "halley/core/graphics/render_target/render_target_texture.h"
#include <gsl/gsl_assert>
#include "graphics/texture.h"

using namespace Halley;

void TextureRenderTarget::setTarget(int attachmentNumber, std::shared_ptr<Texture> tex)
{
	Expects(attachmentNumber >= 0);
	Expects(attachmentNumber < 8);
	Expects(tex);

	if (int(colourBuffer.size()) <= attachmentNumber) {
		colourBuffer.resize(size_t(attachmentNumber) + 1);
		dirty = true;
	}
	if (colourBuffer[attachmentNumber] != tex) {
		colourBuffer[attachmentNumber] = std::move(tex);
		dirty = true;
	}
}

const std::shared_ptr<Texture>& TextureRenderTarget::getTexture(int attachmentNumber) const
{
	return colourBuffer.at(attachmentNumber);
}

void TextureRenderTarget::setDepthTexture(std::shared_ptr<Texture> tex)
{
	if (tex != depthStencilBuffer) {
		depthStencilBuffer = std::move(tex);
		dirty = true;
	}
}

const std::shared_ptr<Texture>& TextureRenderTarget::getDepthTexture() const
{
	return depthStencilBuffer;
}

Rect4i TextureRenderTarget::getViewPort() const
{
	return viewPort ? viewPort.value() : Rect4i(Vector2i(0, 0), colourBuffer.empty() ? Vector2i() : getTexture(0)->getSize());
}

void TextureRenderTarget::setViewPort(Rect4i vp)
{
	viewPort = vp;
}

void TextureRenderTarget::resetViewPort()
{
	viewPort = std::optional<Rect4i>();
}

bool TextureRenderTarget::hasColourBuffer(int attachmentNumber) const
{
	return colourBuffer.size() > static_cast<size_t>(attachmentNumber);
}

bool TextureRenderTarget::hasDepthBuffer() const
{
	return !!depthStencilBuffer;
}
