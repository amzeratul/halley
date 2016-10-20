#pragma once

#include <cstdint>
#include <memory>
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/sprite/sprite.h"
#include <halley/data_structures/flat_map.h>

namespace Halley
{
	class Deserializer;
	class Serializer;

	class Font : public Resource
	{
	public:
		class Glyph
		{
		public:
			int32_t charcode;
			Rect4f area;
			Vector2f size;
			Vector2f horizontalBearing;
			Vector2f verticalBearing;
			Vector2f advance;
			
			Glyph();
			Glyph(const Glyph& other) = default;
			Glyph(Glyph&& other) = default;
			Glyph(int charcode, Rect4f area, Vector2f size, Vector2f horizontalBearing, Vector2f verticalBearing, Vector2f advance);

			Glyph& operator=(const Glyph& o) = default;
			Glyph& operator=(Glyph&& o) = default;

			void serialize(Serializer& serializer) const;
			void deserialize(Deserializer& deserializer);
		};

		Font(String name, String imageName, float ascender, float height, float sizePt);
		Font(String name, String imageName, float ascender, float height, float sizePt, float distanceFieldSmoothRadius);

		explicit Font(ResourceLoader& loader);
		static std::unique_ptr<Font> loadResource(ResourceLoader& loader);

		const Glyph& getGlyph(int code) const;
		float getLineHeightAtSize(float size) const { return height * size / sizePt; }
		float getAscenderDistance() const { return ascender; }
		float getHeight() const { return height; }
		float getSizePoints() const { return sizePt; }
		float getSmoothRadius() const { return smoothRadius; }
		String getName() const { return name; }
		bool isDistanceField() const { return distanceField; }

		void addGlyph(const Glyph& glyph);

		std::shared_ptr<const Material> getMaterial() const;

		void serialize(Serializer& deserializer) const;
		void deserialize(Deserializer& deserializer);

	private:
		String name;
		String imageName;
		float ascender;
		float height;
		float sizePt;
		float smoothRadius;
		bool distanceField;

		std::shared_ptr<Material> material;
		FlatMap<int, Glyph> glyphs;
	};
}
