#pragma once
#include "render_target.h"

namespace Halley
{
	class Texture;

	class TextureRenderTarget : public RenderTarget {
	public:
		virtual ~TextureRenderTarget() {}

		void setTarget(int attachmentNumber, std::shared_ptr<Texture> tex);
		std::shared_ptr<Texture> getTexture(int attachmentNumber) const;

		void setDepthTexture(std::shared_ptr<Texture> tex);
		std::shared_ptr<Texture> getDepthTexture() const;

	protected:
		std::vector<std::shared_ptr<Texture>> attachments;
		std::shared_ptr<Texture> depth;
		bool dirty = false;
	};
}
