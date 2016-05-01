#pragma once

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

		void draw(Painter& painter, Vector2f position, Vector2f align = Vector2f(0, 0)) const;

	private:
		std::shared_ptr<Font> font;
		String text;
		float size = 20;
		float outline = 0;
		Colour colour;
		Colour outlineColour;
	};
}
