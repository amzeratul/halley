#pragma once

#include <memory>
#include <halley/maths/colour.h>
#include <halley/maths/vector2d.h>

namespace Halley
{
	class Font;
	class Painter;

	class TextRenderer
	{
	public:
		TextRenderer();
		explicit TextRenderer(std::shared_ptr<Font> font, String text = "", float size = 20, Colour colour = Colour(1, 1, 1, 1), float outline = 0, Colour outlineColour = Colour(0, 0, 0, 1));

		TextRenderer& setFont(std::shared_ptr<Font> font);
		TextRenderer& setText(String text);
		TextRenderer& setSize(float size);
		TextRenderer& setColour(Colour colour);
		TextRenderer& setOutlineColour(Colour colour);
		TextRenderer& setOutline(float width);
		TextRenderer& setAlignment(float align);
		TextRenderer& setOffset(Vector2f align);

		void draw(Painter& painter, Vector2f position) const;
		Vector2f getExtents() const;

	private:
		std::shared_ptr<Font> font;
		StringUTF32 text;
		float size = 20;
		float outline = 0;
		float align = 0;
		Vector2f offset;
		Colour colour;
		Colour outlineColour;
	};
}
