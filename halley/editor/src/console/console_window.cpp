#include "console_window.h"
#include <halley/core/graphics/sprite/sprite.h>
#include <halley/core/graphics/text/text_renderer.h>
#include <halley/core/graphics/material/material.h>
#include <halley/core/graphics/material/material_definition.h>
#include <halley/core/graphics/material/material_parameter.h>

using namespace Halley;

ConsoleWindow::ConsoleWindow(Resources& resources)
{
	backgroundMaterial = std::make_shared<Material>(resources.get<MaterialDefinition>("distance_field_sprite.yaml"));
	auto& mat = *backgroundMaterial;
	mat["tex0"] = resources.get<Texture>("round_rect.png");
	mat["u_smoothness"] = 0.1f;
	mat["u_outline"] = 0.5f;
	mat["u_outlineColour"] = Colour(0.47f, 0.47f, 0.47f);

	font = resources.get<Font>("consolas.yaml");

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
	Rect4f innerBounds = bounds.shrink(12);
	Rect4f outerBounds = bounds.grow(8);

	// Background
	Sprite()
		.setMaterial(backgroundMaterial)
		.setPos(outerBounds.getTopLeft())
		.setSize(Vector2f(64, 64))
		.setScale(outerBounds.getSize() / Vector2f(64, 64))
		.setTexRect(Rect4f(0, 0, 1, 1))
		.setColour(Colour4f(0.0f, 0.0f, 0.0f, 0.4f))
		.drawSliced(painter, Vector4f(0.45f, 0.45f, 0.45f, 0.45f));

	const float size = 14;
	float lineH = font->getLineHeightAtSize(size);
	int nLines = int(innerBounds.getHeight() / lineH) - 1;
	Vector2f cursor = innerBounds.getBottomLeft();

	TextRenderer text;
	text.setFont(font).setSize(size).setOffset(Vector2f(0, 1)).setColour(Colour(1, 1, 1));

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
	history.push_back(input);
	input = "";
}
