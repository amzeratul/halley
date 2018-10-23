#pragma once

#include <memory>
#include <halley/maths/colour.h>
#include <halley/maths/vector2.h>
#include "halley/maths/rect.h"
#include "halley/data_structures/maybe.h"
#include <gsl/span>

namespace Halley
{
	class LocalisedString;
	class Font;
	class Painter;
	class Material;
	class Sprite;

	using ColourOverride = std::pair<size_t, Maybe<Colour4f>>;

	class TextRenderer
	{
	public:
		using SpriteFilter = std::function<void(gsl::span<Sprite>)>;

		TextRenderer();
		explicit TextRenderer(std::shared_ptr<const Font> font, String text = "", float size = 20, Colour colour = Colour(1, 1, 1, 1), float outline = 0, Colour outlineColour = Colour(0, 0, 0, 1));

		TextRenderer& setPosition(Vector2f pos);
		TextRenderer& setFont(std::shared_ptr<const Font> font);
		TextRenderer& setText(const String& text);
		TextRenderer& setText(const StringUTF32& text);
		TextRenderer& setText(const LocalisedString& text);
		TextRenderer& setSize(float size);
		TextRenderer& setColour(Colour colour);
		TextRenderer& setOutlineColour(Colour colour);
		TextRenderer& setOutline(float width);
		TextRenderer& setAlignment(float align);
		TextRenderer& setOffset(Vector2f align);
		TextRenderer& setClip(Rect4f clip);
		TextRenderer& setClip();
		TextRenderer& setSmoothness(float smoothness);
		TextRenderer& setPixelOffset(Vector2f offset);
		TextRenderer& setColourOverride(const std::vector<ColourOverride>& colOverride);
		TextRenderer& setLineSpacing(float spacing);

		TextRenderer clone() const;

		void generateSprites(std::vector<Sprite>& sprites) const;
		void draw(Painter& painter) const;

		void setSpriteFilter(SpriteFilter f);

		Vector2f getExtents() const;
		Vector2f getExtents(const StringUTF32& str) const;
		Vector2f getCharacterPosition(size_t character) const;
		Vector2f getCharacterPosition(size_t character, const StringUTF32& str) const;
		size_t getCharacterAt(const Vector2f& position) const;
		size_t getCharacterAt(const Vector2f& position, const StringUTF32& str) const;

		StringUTF32 split(const String& str, float width) const;
		StringUTF32 split(const StringUTF32& str, float width) const;
		StringUTF32 split(float width) const;

		Vector2f getPosition() const;
		String getText() const;
		Colour getColour() const;
		Colour getOutlineColour() const;
		float getSmoothness() const;
		Maybe<Rect4f> getClip() const;
		float getLineHeight() const;
		float getAlignment() const;
		float getScale() const;

	private:
		std::shared_ptr<const Font> font;
		std::shared_ptr<Material> material;
		StringUTF32 text;
		SpriteFilter spriteFilter;
		
		float size = 20;
		float outline = 0;
		float align = 0;
		float smoothness = 1.0f;
		float lineSpacing = 0.0f;

		Vector2f position;
		Vector2f offset;
		Vector2f pixelOffset;
		Colour colour;
		Colour outlineColour;
		Maybe<Rect4f> clip;

		std::vector<ColourOverride> colourOverrides;

		mutable Vector<Sprite> spritesCache;
		mutable bool materialDirty = true;
		mutable bool glyphsDirty = true;
		mutable bool positionDirty = true;
	};
}
