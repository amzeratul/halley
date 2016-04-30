#include "text_renderer.h"
#include "font.h"
#include "../painter.h"
#include "../material/material.h"
#include "../material/material_parameter.h"

using namespace Halley;

TextRenderer::TextRenderer()
{
}

TextRenderer::TextRenderer(std::shared_ptr<Font> font, String text, float size, Colour colour, float outline, Colour outlineColour)
	: font(font)
	, text(text)
	, size(size)
	, outline(outline)
	, colour(colour)
	, outlineColour(outlineColour)
{
}

TextRenderer& TextRenderer::setFont(std::shared_ptr<Font> v)
{
	font = v;
	return *this;
}

TextRenderer& TextRenderer::setText(String v)
{
	text = v;
	return *this;
}

TextRenderer& TextRenderer::setSize(float v)
{
	size = v;
	return *this;
}

TextRenderer& TextRenderer::setColour(Colour v)
{
	colour = v;
	return *this;
}

TextRenderer& TextRenderer::setOutline(float v0, Colour v1)
{
	outline = v0;
	outlineColour = v1;
	return *this;
}

void TextRenderer::draw(Painter& painter, Vector2f position, Vector2f align) const
{
	assert(font);
	auto material = font->getMaterial()->clone();
	float scale = 4 * size / font->getSizePoints();
	float smooth = clamp(1.0f / (scale * font->getSmoothRadius()), 0.01f, 0.99f);

	(*material)["u_smoothness"] = smooth;
	(*material)["u_outline"] = outline;
	(*material)["u_outlineColour"] = outlineColour;

	Vector2f p = position;

	auto chars = text.getUTF32();
	for (auto& c : chars) {
		if (c == '\n') {
			// Line break!
			p.x = position.x;
			p.y += font->getLineDistance() * scale;
		} else {
			auto& glyph = font->getGlyph(c);

			auto sprite = Sprite()
				.setTexRect(glyph.area)
				.setMaterial(material)
				.setColour(colour)
				.setPivot(glyph.horizontalBearing / glyph.size * Vector2f(-1, 1))
				.setSize(glyph.size)
				.setScale(scale)
				.setPos(p);

			sprite.draw(painter);

			p += Vector2f(glyph.advance.x, 0) * scale;
		}
	}
}
