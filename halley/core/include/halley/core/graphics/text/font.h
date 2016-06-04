#pragma once

#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/sprite/sprite.h"
#include <memory>
#include <halley/data_structures/flat_map.h>

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
			
			Glyph(Glyph&& other) = default;
			Glyph(int charcode, Rect4f area, Vector2f size, Vector2f horizontalBearing, Vector2f verticalBearing, Vector2f advance);

			Glyph& operator=(const Glyph& o) = default;
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
		FlatMap<int, Glyph> glyphs;
	};
}
