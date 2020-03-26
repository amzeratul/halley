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

	class RenderSurface {
	public:
		RenderSurface(VideoAPI& video, Resources& resources, const String& materialName = "Halley/SpriteOpaque");
		
		void setSize(Vector2i size);

		bool isReady() const;
		Sprite getSurfaceSprite() const;

		TextureRenderTarget& getRenderTarget() const;

	private:
		VideoAPI& video;
		Resources& resources;

		std::shared_ptr<TextureRenderTarget> renderTarget;
		Vector2i curTextureSize;
		Vector2i curRenderSize;

		std::shared_ptr<Material> material;
	};
}
