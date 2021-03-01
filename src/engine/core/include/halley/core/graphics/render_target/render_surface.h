#pragma once
#include <memory>
#include <memory>

#include "halley/maths/vector2.h"
#include "halley/text/halleystring.h"

namespace Halley {
	class VideoAPI;
	class Material;
	class TextureRenderTarget;
	class Sprite;
	class Resources;

	struct RenderSurfaceOptions {
		bool useFiltering = false;
		bool createDepthStencil = true;
		String name;
	};

	class RenderSurface {
	public:
		explicit RenderSurface(VideoAPI& video, RenderSurfaceOptions options = {});
		RenderSurface(VideoAPI& video, Resources& resources, const String& materialName = "Halley/SpriteOpaque", RenderSurfaceOptions options = {});
		
		void setSize(Vector2i size);

		bool isReady() const;
		Sprite getSurfaceSprite() const;

		TextureRenderTarget& getRenderTarget() const;

	private:
		VideoAPI& video;
		RenderSurfaceOptions options;

		std::shared_ptr<TextureRenderTarget> renderTarget;
		Vector2i curTextureSize;
		Vector2i curRenderSize;

		std::shared_ptr<Material> material;

		int version = 0;
	};
}
