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
			Vector2f horizontalBearing;
			Vector2f verticalBearing;
			Vector2f advance;
			
			Glyph(Glyph&& other);
			Glyph(int charcode, Rect4f area, Vector2f horizontalBearing, Vector2f verticalBearing, Vector2f advance);
		};

		explicit Font(ResourceLoader& loader);
		static std::unique_ptr<Font> loadResource(ResourceLoader& loader);

		const Glyph& getGlyph(int code) const;
		float getLineDistance() const { return height; }
		float getSizePoints() const { return sizePt; }
		String getName() const { return name; }

		std::shared_ptr<const Material> getMaterial() const;

	private:
		String name;
		float height;
		float sizePt;

		std::shared_ptr<Material> material;
		std::map<int, Glyph> glyphs;
	};
}
