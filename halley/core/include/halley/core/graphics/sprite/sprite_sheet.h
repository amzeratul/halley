#pragma once

#include <halley/maths/vector2d.h>
#include <halley/maths/rect.h>
#include <memory>
#include <halley/resources/resource.h>
#include <halley/text/halleystring.h>
#include <halley/data_structures/hash_map.h>

namespace Halley
{
	class Texture;
	class ResourceLoader;

	class SpriteSheetEntry
	{
	public:
		Vector2f pivot;
		Vector2f size;
		Rect4f coords;
		bool rotated;
	};
	
	class SpriteSheet : public Resource
	{
	public:
		std::shared_ptr<Texture> getTexture() const { return texture; }
		const SpriteSheetEntry& getSprite(String name) const;

		static std::unique_ptr<SpriteSheet> loadResource(ResourceLoader& loader);

	private:
		std::shared_ptr<Texture> texture;
		HashMap<String, SpriteSheetEntry> sprites;
	};
}
