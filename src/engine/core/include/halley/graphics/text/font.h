#pragma once

#include <cstdint>
#include <memory>
#include "halley/graphics/texture.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/data_structures/hash_map.h"

namespace Halley
{
	class Deserializer;
	class Serializer;

	class Font final : public Resource
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
			Glyph(Glyph&& other) noexcept = default;
			Glyph(int charcode, Rect4f area, Vector2f size, Vector2f horizontalBearing, Vector2f verticalBearing, Vector2f advance);

			Glyph& operator=(const Glyph& o) = default;
			Glyph& operator=(Glyph&& o) noexcept = default;

			void serialize(Serializer& serializer) const;
			void deserialize(Deserializer& deserializer);
		};

		Font() = default;
		Font(String name, String imageName, float ascender, float height, float sizePt, float replacementScale, Vector2i imageSize);
		Font(String name, String imageName, float ascender, float height, float sizePt, float replacementScale, Vector2i imageSize, float distanceFieldSmoothRadius, Vector<String> fallback, bool floorGlyphPosition);

		static std::unique_ptr<Font> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::Font; }
		void reload(Resource&& resource) override;
		void onLoaded(Resources& resources) override;

		std::pair<const Glyph&, const Font&> getGlyph(int code) const;
		const Glyph& getGlyphHere(int code) const;
		const Font& getFontForGlyph(int code) const;
		float getLineHeightAtSize(float size) const;
		float getAscenderDistance() const;
		float getHeight() const;
		float getSizePoints() const;
		float getSmoothRadius() const;
		Vector2i getImageSize() const;
		float getReplacementScale() const;
		String getName() const;
		bool isDistanceField() const;
		bool shouldFloorGlyphPosition() const;

		void addGlyph(const Glyph& glyph);

		std::shared_ptr<Material> getMaterial() const;

		void serialize(Serializer& deserializer) const;
		void deserialize(Deserializer& deserializer);

		void printGlyphs() const;
		static void setFallbackFontsIgnore(const HashSet<String>&& ignore);

	private:
		String name;
		String imageName;
		float ascender;
		float height;
		float sizePt;
		float smoothRadius;
		float replacementScale = 1.0f;
		Vector2i imageSize;
		bool distanceField;
		Vector<std::shared_ptr<const Font>> fallbackFont;
		Vector<String> fallback;
		static HashSet<String> fallbackIgnore;
		bool floorGlyphPosition;

		std::shared_ptr<Material> material;
		HashMap<int, Glyph> glyphs;
	};
	
}
