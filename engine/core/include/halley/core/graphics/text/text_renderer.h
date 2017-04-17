#pragma once

#include <memory>
#include <halley/maths/colour.h>
#include <halley/maths/vector2.h>
#include "halley/maths/rect.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class Font;
	class Painter;
	class Material;

	class TextRenderer
	{
	public:
		TextRenderer();
		explicit TextRenderer(std::shared_ptr<const Font> font, String text = "", float size = 20, Colour colour = Colour(1, 1, 1, 1), float outline = 0, Colour outlineColour = Colour(0, 0, 0, 1));

		TextRenderer& setPosition(Vector2f pos);
		TextRenderer& setFont(std::shared_ptr<const Font> font);
		TextRenderer& setText(const String& text);
		TextRenderer& setText(const StringUTF32& text);
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

		TextRenderer clone() const;

		void draw(Painter& painter) const;
		Vector2f getExtents() const;

		StringUTF32 split(const String& str, float width) const;
		StringUTF32 split(const StringUTF32& str, float width) const;
		StringUTF32 split(float width) const;

		Vector2f getPosition() const;
		String getText() const;
		Colour getColour() const;
		Colour getOutlineColour() const;
		float getSmoothness() const;

	private:
		std::shared_ptr<const Font> font;
		std::shared_ptr<Material> material;
		StringUTF32 text;
		float size = 20;
		float outline = 0;
		float align = 0;
		float smoothness = 1.0f;
		Vector2f position;
		Vector2f offset;
		Vector2f pixelOffset;
		Colour colour;
		Colour outlineColour;
		Maybe<Rect4f> clip;
	};
}
