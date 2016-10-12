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
	};

	class SpriteSheetFrameTag
	{
	public:
		String name;
		int from;
		int to;
	};
	
	class SpriteSheet : public Resource
	{
	public:
		std::shared_ptr<Texture> getTexture() const { return texture; }
		const SpriteSheetEntry& getSprite(String name) const;
		const std::vector<SpriteSheetFrameTag>& getFrameTags() const;
		std::vector<String> getSpriteNames() const;

		void loadJson(gsl::span<const gsl::byte> data);

		static std::unique_ptr<SpriteSheet> loadResource(ResourceLoader& loader);

	private:
		std::shared_ptr<Texture> texture;
		HashMap<String, SpriteSheetEntry> sprites;
		std::vector<SpriteSheetFrameTag> frameTags;
		String textureName;
	};
}
