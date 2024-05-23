#pragma once

#include "halley/resources/resource.h"
#include "halley/maths/vector2.h"
#include "halley/file_formats/image_mask.h"
#include "texture_descriptor.h"
#include <memory>

namespace Halley
{
	class Painter;
	class TextureDescriptor;
	class ResourceLoader;

	class ImageDataAndMask {
	public:
		Bytes imageData;
		ImageMask mask;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class Texture : public AsyncResource
	{
	public:
		Texture(Vector2i size);

		void load(TextureDescriptor descriptor);

		static std::shared_ptr<Texture> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Texture; }

		Vector2i getSize() const { return size; }
		const TextureDescriptor& getDescriptor() const { return descriptor; }

		void copyToTexture(Painter& painter, Texture& other) const;
		void copyToImage(Painter& painter, Image& image) const;
		std::unique_ptr<Image> makeImage(Painter& painter) const;

		const Image* tryGetOriginalImage() const;

		virtual void generateMipMaps();

		ResourceMemoryUsage getMemoryUsage() const override;

		void setAlphaMask(ImageMask mask);
		bool hasOpaquePixels(Rect4i pixelBounds) const;

	protected:
		Vector2i size;
		TextureDescriptor descriptor;
		ImageMask mask;

		virtual void doLoad(TextureDescriptor& descriptor);
		virtual void doCopyToTexture(Painter& painter, Texture& other) const;
		virtual void doCopyToImage(Painter& painter, Image& image) const;
		virtual size_t getVRamUsage() const;

		void moveFrom(Texture& other);
	};
}
