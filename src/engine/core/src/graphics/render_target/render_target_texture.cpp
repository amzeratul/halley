#include "halley/core/graphics/render_target/render_target_texture.h"
#include <gsl/gsl_assert>
#include "graphics/texture.h"

using namespace Halley;

void TextureRenderTarget::setTarget(int attachmentNumber, std::shared_ptr<Texture> tex)
{
	Expects(attachmentNumber >= 0);
	Expects(attachmentNumber < 8);
	Expects(tex);

	if (int(attachments.size()) <= attachmentNumber) {
		attachments.resize(size_t(attachmentNumber) + 1);
		dirty = true;
	}
	if (attachments[attachmentNumber] != tex) {
		attachments[attachmentNumber] = std::move(tex);
		dirty = true;
	}
}

const std::shared_ptr<Texture>& TextureRenderTarget::getTexture(int attachmentNumber) const
{
	return attachments.at(attachmentNumber);
}

void TextureRenderTarget::setDepthTexture(std::shared_ptr<Texture> tex)
{
	if (tex != depth) {
		depth = std::move(tex);
		dirty = true;
	}
}

const std::shared_ptr<Texture>& TextureRenderTarget::getDepthTexture() const
{
	return depth;
}

Rect4i TextureRenderTarget::getViewPort() const
{
	return viewPort ? viewPort.value() : Rect4i(Vector2i(0, 0), getTexture(0)->getSize());
}

void TextureRenderTarget::setViewPort(Rect4i vp)
{
	viewPort = vp;
}

void TextureRenderTarget::resetViewPort()
{
	viewPort = std::optional<Rect4i>();
}
