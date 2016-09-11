#include "halley/core/graphics/render_target/render_target_texture.h"
#include <gsl/gsl_assert>

using namespace Halley;

void TextureRenderTarget::setTarget(int attachmentNumber, std::shared_ptr<Texture> tex)
{
	Expects(attachmentNumber >= 0);
	Expects(attachmentNumber < 8);
	Expects(tex != std::shared_ptr<Texture>());

	if (attachments.size() <= attachmentNumber) {
		attachments.resize(attachmentNumber + 1);
	}
	attachments[attachmentNumber] = tex;

	dirty = true;
}

std::shared_ptr<Texture> TextureRenderTarget::getTexture(int attachmentNumber) const
{
	return attachments.at(attachmentNumber);
}

void TextureRenderTarget::setDepthTexture(std::shared_ptr<Texture> tex)
{
	depth = tex;
	dirty = true;
}

std::shared_ptr<Texture> TextureRenderTarget::getDepthTexture() const
{
	return depth;
}
