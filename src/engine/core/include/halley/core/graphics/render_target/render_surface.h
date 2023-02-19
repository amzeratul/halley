#pragma once
#include <memory>
#include <memory>

#include "halley/maths/vector2.h"
#include "halley/text/halleystring.h"

namespace Halley {
	class Texture;
	class VideoAPI;
	class Material;
	class TextureRenderTarget;
	class Sprite;
	class Resources;

	struct RenderSurfaceOptions {
		bool useFiltering = false;
		bool createDepthStencil = true;
		bool powerOfTwo = true;
		String name;
	};

	class RenderSurface {
	public:
		explicit RenderSurface(VideoAPI& video, RenderSurfaceOptions options = {});
		RenderSurface(VideoAPI& video, Resources& resources, const String& materialName = "Halley/SpriteOpaque", RenderSurfaceOptions options = {});
		
		void setSize(Vector2i size);

		bool isReady() const;
		Sprite getSurfaceSprite() const;
		Sprite getSurfaceSprite(std::shared_ptr<Material> material) const;

		TextureRenderTarget& getRenderTarget() const;

		void createNewColourTarget();
		void createNewDepthStencilTarget();
		void setColourTarget(std::shared_ptr<Texture> texture);
		void setDepthStencilTarget(std::shared_ptr<Texture> texture);


	private:
		VideoAPI& video;
		RenderSurfaceOptions options;

		std::shared_ptr<TextureRenderTarget> renderTarget;
		Vector2i curTextureSize;
		Vector2i curRenderSize;

		std::shared_ptr<Material> material;

		int version = 0;
		bool hasColourTarget = false;
		bool hasDepthStencil = false;
	};
}
