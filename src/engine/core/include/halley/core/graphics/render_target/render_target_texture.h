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
		const std::shared_ptr<Texture>& getTexture(int attachmentNumber) const;

		void setDepthTexture(std::shared_ptr<Texture> tex);
		const std::shared_ptr<Texture>& getDepthTexture() const;

		Rect4i getViewPort() const override;
		void setViewPort(Rect4i viewPort);
		void resetViewPort();

		bool hasColourBuffer(int attachmentNumber) const override;
		bool hasDepthBuffer() const override;
	
	protected:
		Vector<std::shared_ptr<Texture>> colourBuffer;
		std::shared_ptr<Texture> depthStencilBuffer;
		std::optional<Rect4i> viewPort;
		bool dirty = false;
	};
}
