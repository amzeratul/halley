#pragma once

#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <memory>
#include <halley/resources/resource.h>
#include <halley/text/halleystring.h>
#include <halley/data_structures/hash_map.h>
#include <gsl/span>

namespace Halley
{
	class Resources;
	class Serializer;
	class Deserializer;
	class ResourceDataStatic;
	class Texture;
	class ResourceLoader;

	class SpriteSheetEntry
	{
	public:
		Vector2f pivot;
		Vector2f size;
		Rect4f coords;
		int duration;
		bool rotated;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class SpriteSheetFrameTag
	{
	public:
		String name;
		int from = 0;
		int to = 0;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};
	
	class SpriteSheet : public Resource
	{
	public:
		const std::shared_ptr<const Texture>& getTexture() const { return texture; }
		const SpriteSheetEntry& getSprite(const String& name) const;
		const SpriteSheetEntry& getSprite(size_t idx) const;

		const std::vector<SpriteSheetFrameTag>& getFrameTags() const;
		std::vector<String> getSpriteNames() const;

		size_t getSpriteCount() const;
		size_t getIndex(const String& name) const;

		void loadJson(gsl::span<const gsl::byte> data);
		void loadTexture(Resources& resources);

		void addSprite(String name, const SpriteSheetEntry& sprite);
		void setTextureName(String name);

		static std::unique_ptr<SpriteSheet> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::SpriteSheet; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void setPivot(Vector2i pivot);
		Vector2i getPivot() const;

	private:
		std::shared_ptr<const Texture> texture;
		std::vector<SpriteSheetEntry> sprites;
		HashMap<String, uint32_t> spriteIdx;
		std::vector<SpriteSheetFrameTag> frameTags;
		String textureName;
		Vector2i pivot;
	};
}
