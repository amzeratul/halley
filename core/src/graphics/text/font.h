#pragma once

#include "../texture.h"
#include "../sprite/sprite.h"

namespace Halley
{
	class Font : public Resource
	{
	public:
		class Glyph
		{
		public:
			int charcode;
			Rect4f area;
			Vector2f size;
			Vector2f horizontalBearing;
			Vector2f verticalBearing;
			Vector2f advance;
			
			Glyph(Glyph&& other);
			Glyph(int charcode, Rect4f area, Vector2f size, Vector2f horizontalBearing, Vector2f verticalBearing, Vector2f advance);
		};

		explicit Font(ResourceLoader& loader);
		static std::unique_ptr<Font> loadResource(ResourceLoader& loader);

		const Glyph& getGlyph(int code) const;
		float getAscenderDistance() const { return ascender; }
		float getHeight() const { return height; }
		float getSizePoints() const { return sizePt; }
		float getSmoothRadius() const { return smoothRadius; }
		String getName() const { return name; }

		std::shared_ptr<const Material> getMaterial() const;

	private:
		String name;
		float ascender;
		float height;
		float sizePt;
		float smoothRadius;

		std::shared_ptr<Material> material;
		std::map<int, Glyph> glyphs;
	};
}
