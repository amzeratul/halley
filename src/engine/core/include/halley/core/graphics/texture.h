#pragma once

#include "halley/resources/resource.h"
#include "halley/maths/vector2.h"
#include "texture_descriptor.h"
#include <memory>

namespace Halley
{
	class Painter;
	class TextureDescriptor;
	class ResourceLoader;

	class Texture : public AsyncResource
	{
	public:
		Texture(Vector2i size);

		void load(TextureDescriptor descriptor);

		std::optional<uint32_t> getPixel(Vector2f texPos) const;

		static std::shared_ptr<Texture> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Texture; }

		Vector2i getSize() const { return size; }
		const TextureDescriptor& getDescriptor() const { return descriptor; }

		void copyToTexture(Painter& painter, Texture& other) const;
		void copyToImage(Painter& painter, Image& image) const;
		std::unique_ptr<Image> makeImage(Painter& painter) const;

	protected:
		Vector2i size;
		TextureDescriptor descriptor;

		virtual void doLoad(TextureDescriptor& descriptor);
		virtual void doCopyToTexture(Painter& painter, Texture& other) const;
		virtual void doCopyToImage(Painter& painter, Image& image) const;
	};
}
