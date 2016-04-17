#pragma once

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
		std::map<String, SpriteSheetEntry> sprites;
	};
}
