#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/sprite/sprite.h"

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

		Font(String name, String imageName, float ascender, float height, float sizePt, float replacementScale, Vector2i imageSize);
		Font(String name, String imageName, float ascender, float height, float sizePt, float replacementScale, Vector2i imageSize, float distanceFieldSmoothRadius, std::vector<String> fallback);

		explicit Font(ResourceLoader& loader);

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

		void addGlyph(const Glyph& glyph);

		std::shared_ptr<Material> getMaterial() const;

		void serialize(Serializer& deserializer) const;
		void deserialize(Deserializer& deserializer);

		void printGlyphs() const;

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
		std::vector<std::shared_ptr<const Font>> fallbackFont;
		std::vector<String> fallback;

		std::shared_ptr<Material> material;
		std::unordered_map<int, Glyph> glyphs;
	};
}
