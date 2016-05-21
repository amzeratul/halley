#include <cassert>
#include "halley/graphics/render_target/render_target_texture.h"

using namespace Halley;

void TextureRenderTarget::setTarget(int attachmentNumber, std::shared_ptr<Texture> tex)
{
	assert(attachmentNumber >= 0);
	assert(attachmentNumber < 8);
	assert(tex != std::shared_ptr<Texture>());

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
