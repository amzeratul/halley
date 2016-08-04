#include "console_window.h"

using namespace Halley;

ConsoleWindow::ConsoleWindow(Resources& resources)
{
	backgroundMaterial = std::make_shared<Material>(resources.get<MaterialDefinition>("solid_colour.yaml"));
	font = resources.get<Font>("ubuntub.yaml");

	buffer.push_back("Halley Engine Editor.");
	buffer.push_back("Hello world!");
}

void ConsoleWindow::update(InputKeyboard& keyboard)
{
	for (int next; (next = keyboard.getNextLetter()) != 0;) {
		if (next == '\b') {
			input.setSize(std::max(0, int(input.size()) - 1));
		} else if (next == '\n') {
			submit();
		} else {
			input.appendCharacter(next);
		}
	}
}

void ConsoleWindow::draw(Painter& painter, Rect4f bounds) const
{
	// Background
	Sprite()
		.setMaterial(backgroundMaterial)
		.setPos(bounds.getTopLeft())
		.setSize(bounds.getSize())
		.setColour(Colour4f(0, 0, 0, 0.3f))
		.draw(painter);

	const float size = 16;
	float lineH = font->getLineHeightAtSize(size);
	int nLines = int(bounds.getHeight() / lineH) - 1;

	TextRenderer text;
	Vector2f cursor = bounds.getBottomLeft();
	text.setFont(font).setSize(size).setOffset(Vector2f(0, 1)).setColour(Colour(0.61f, 1.0f, 0.75f));

	// Draw command
	text.setText("> " + input).draw(painter, cursor);
	cursor += Vector2f(0, -lineH);

	// Draw buffer
	int last = int(buffer.size());
	int first = std::max(0, last - nLines);
	for (int i = last; --i >= first;) {
		text.setText(buffer[i]).draw(painter, cursor);
		cursor += Vector2f(0, -lineH);
	}
}

void ConsoleWindow::submit()
{
	buffer.push_back("> " + input);
	input = "";
}
