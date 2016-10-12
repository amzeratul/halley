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
		int from;
		int to;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};
	
	class SpriteSheet : public Resource
	{
	public:
		std::shared_ptr<Texture> getTexture() const { return texture; }
		const SpriteSheetEntry& getSprite(String name) const;
		const std::vector<SpriteSheetFrameTag>& getFrameTags() const;
		std::vector<String> getSpriteNames() const;

		void loadJson(gsl::span<const gsl::byte> data);
		void loadTexture(Resources& resources);

		void addSprite(String name, const SpriteSheetEntry& sprite);
		void setTextureName(String name);

		static std::unique_ptr<SpriteSheet> loadResource(ResourceLoader& loader);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		std::shared_ptr<Texture> texture;
		HashMap<String, SpriteSheetEntry> sprites;
		std::vector<SpriteSheetFrameTag> frameTags;
		String textureName;
	};
}
