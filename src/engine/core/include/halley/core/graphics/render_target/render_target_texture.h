#pragma once
#include "render_target.h"
#include "halley/data_structures/maybe.h"

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

		Rect4i getViewPort() const override;
		void setViewPort(Rect4i viewPort);
		void resetViewPort();

	protected:
		Vector<std::shared_ptr<Texture>> attachments;
		std::shared_ptr<Texture> depth;
		bool dirty = false;
		Maybe<Rect4i> viewPort;
	};
}
