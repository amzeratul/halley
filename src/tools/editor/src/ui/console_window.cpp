#include "console_window.h"
#include <halley/core/graphics/sprite/sprite.h>
#include <halley/core/graphics/text/text_renderer.h>
#include <halley/core/graphics/material/material.h>
#include <halley/core/graphics/material/material_parameter.h>

using namespace Halley;

ConsoleWindow::ConsoleWindow(UIFactory& ui)
{
	auto& resources = ui.getResources();

	background = Sprite()
		.setImage(resources, "round_rect.png", "Halley/DistanceFieldSprite")
		.setColour(Colour4f(0.0f, 0.0f, 0.0f, 0.4f))
		.setPivot(Vector2f(0, 0));

	background.getMaterial()
		.set("tex0", resources.get<Texture>("round_rect.png"))
		.set("u_smoothness", 1.0f / 16.0f)
		.set("u_outline", 0.5f)
		.set("u_outlineColour", Colour(0.47f, 0.47f, 0.47f));

	font = resources.get<Font>("Inconsolata Medium");

	printLn("Welcome to the Halley Game Engine Editor.");
	Logger::addSink(*this);
}

ConsoleWindow::~ConsoleWindow()
{
	Logger::removeSink(*this);
}

void ConsoleWindow::log(LoggerLevel, const String& msg)
{
	std::unique_lock<std::mutex> lock(mutex);
	printLn(msg);
}

void ConsoleWindow::draw(UIPainter& painter) const
{
	std::unique_lock<std::mutex> lock(mutex);

	const Rect4f bounds = Rect4f(getPosition(), getPosition() + getSize());
	const Rect4f innerBounds = bounds.shrink(20);
	const Rect4f outerBounds = bounds;

	// Background
	painter.draw(background.clone()
		.setPos(outerBounds.getTopLeft())
		.scaleTo(outerBounds.getSize()), true);

	const float size = 18;
	float lineH = font->getLineHeightAtSize(size);
	int nLines = int(innerBounds.getHeight() / lineH) - 1;
	Vector2f cursor = innerBounds.getBottomLeft();

	TextRenderer text;
	text.setFont(font).setSize(size).setOffset(Vector2f(0, 1)).setColour(Colour(1, 1, 1));

	// Draw command
	painter.draw(text.setText("> " + input).setPosition(cursor), true);
	cursor += Vector2f(0, -lineH);

	// Draw buffer
	int last = int(buffer.size());
	int nDrawn = 0;
	for (int i = last; --i >= 0 && nDrawn < nLines;) {
		int nLinesHere = 1;
		size_t pos = 0;
		while ((pos = buffer[i].find('\n', pos)) != size_t(-1)) {
			nLinesHere++;
			pos++;
		}
		text.setText(buffer[i]);
		//auto extents = text.getExtents();
		painter.draw(text.setPosition(cursor), true);
		cursor += Vector2f(0, -lineH * nLinesHere);
		nDrawn += nLinesHere;
	}
}

void ConsoleWindow::submit()
{
	printLn("> " + input);
	history.push_back(input);
	input = "";
}

void ConsoleWindow::printLn(const String& line)
{
	buffer.push_back(line);
}