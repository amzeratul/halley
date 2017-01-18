#include "graphics/text/text_renderer.h"
#include "graphics/text/font.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <gsl/gsl_assert>

using namespace Halley;

TextRenderer::TextRenderer()
{
}

TextRenderer::TextRenderer(std::shared_ptr<const Font> font, String text, float size, Colour colour, float outline, Colour outlineColour)
	: font(font)
	, text(text.getUTF32())
	, size(size)
	, outline(outline)
	, colour(colour)
	, outlineColour(outlineColour)
{
}

TextRenderer& TextRenderer::setPosition(Vector2f pos)
{
	position = pos;
	return *this;
}

TextRenderer& TextRenderer::setFont(std::shared_ptr<const Font> v)
{
	font = v;
	return *this;
}

TextRenderer& TextRenderer::setText(String v)
{
	text = v.getUTF32();
	text.push_back('\n');
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

TextRenderer& TextRenderer::setOutline(float v)
{
	outline = v;
	return *this;
}

TextRenderer& TextRenderer::setAlignment(float v)
{
	align = v;
	return *this;
}

TextRenderer& TextRenderer::setOffset(Vector2f v)
{
	offset = v;
	return *this;
}

TextRenderer TextRenderer::clone() const
{
	return *this;
}

TextRenderer& TextRenderer::setOutlineColour(Colour v)
{
	outlineColour = v;
	return *this;
}

void TextRenderer::draw(Painter& painter) const
{
	Expects(font);
	auto material = font->getMaterial()->clone();
	
	if (font->isDistanceField()) {
		float smooth = clamp(1.0f / font->getSmoothRadius(), 0.001f, 0.999f);
		float outlineSize = clamp(outline / font->getSmoothRadius(), 0.0f, 0.995f);
		material
			->set("u_smoothness", smooth)
			.set("u_outline", outlineSize)
			.set("u_outlineColour", outlineColour);
	}

	float scale = size / font->getSizePoints();
	Vector2f p = position + Vector2f(0, font->getAscenderDistance() * scale);
	if (offset != Vector2f(0, 0)) {
		p -= (getExtents() * offset).floor();
	}

	Vector<Sprite> sprites;

	size_t startPos = 0;
	Vector2f lineOffset;

	const size_t n = text.size();
	for (size_t i = 0; i < n; i++) {
		int c = text[i];
		
		if (c == '\n') {
			// Line break, update previous characters!
			if (align != 0) {
				Vector2f off = (-lineOffset * align).floor();
				for (size_t j = startPos; j < sprites.size(); j++) {
					auto& sprite = sprites[j];
					sprite.setPos(sprite.getPosition() + off);
				}
			}

			// Move pen
			p.y += font->getHeight() * scale;

			// Reset
			startPos = sprites.size();
			lineOffset.x = 0;
		} else {
			auto& glyph = font->getGlyph(c);

			sprites.push_back(Sprite()
				.setMaterial(material)
				.setSize(glyph.size)
				.setTexRect(glyph.area)
				.setColour(colour)
				.setPivot(glyph.horizontalBearing / glyph.size * Vector2f(-1, 1))
				.setScale(scale)
				.setPos(p + lineOffset));

			lineOffset.x += glyph.advance.x * scale;
		}
	}

	Sprite::draw(sprites.data(), sprites.size(), painter);
}

Vector2f TextRenderer::getExtents() const
{
	Vector2f p;
	float w = 0;
	float scale = size / font->getSizePoints();
	float lineH = font->getHeight() * scale;

	for (auto& c : text) {
		if (c == '\n') {
			// Line break!
			w = std::max(w, p.x);
			p.x = 0;
			p.y += lineH;
		}
		else {
			auto& glyph = font->getGlyph(c);
			p += Vector2f(glyph.advance.x, 0) * scale;
		}
	}
	w = std::max(w, p.x);

	return Vector2f(w, p.y);
}

Vector2f TextRenderer::getPosition() const
{
	return position;
}
