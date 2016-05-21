#include "text_renderer.h"
#include "font.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_parameter.h"

using namespace Halley;

TextRenderer::TextRenderer()
{
}

TextRenderer::TextRenderer(std::shared_ptr<Font> font, String text, float size, Colour colour, float outline, Colour outlineColour)
	: font(font)
	, text(text.getUTF32())
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

TextRenderer& TextRenderer::setOutlineColour(Colour v)
{
	outlineColour = v;
	return *this;
}

void TextRenderer::draw(Painter& painter, Vector2f position) const
{
	assert(font);
	auto material = font->getMaterial()->clone();
	float scale = size / font->getSizePoints();
	float smooth = clamp(1.0f / (scale * font->getSmoothRadius()), 0.01f, 0.99f);
	float outlineSize = clamp(outline / (scale * font->getSmoothRadius()), 0.0f, 0.95f);

	(*material)["u_smoothness"] = smooth;
	(*material)["u_outline"] = outlineSize;
	(*material)["u_outlineColour"] = outlineColour;

	Vector2f p = position + Vector2f(0, font->getAscenderDistance() * scale);
	if (offset != Vector2f(0, 0)) {
		p -= getExtents() * offset;
	}

	std::vector<Sprite> sprites;

	size_t startPos = 0;
	Vector2f lineOffset;

	const size_t n = text.size();
	for (size_t i = 0; i < n; i++) {
		int c = text[i];
		
		if (c == '\n') {
			// Line break, update previous characters!
			if (align != 0) {
				for (size_t j = startPos; j < sprites.size(); j++) {
					auto& sprite = sprites[j];
					sprite.setPos(sprite.getPosition() - lineOffset * align);
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
				.setTexRect(glyph.area)
				.setMaterial(material)
				.setColour(colour)
				.setPivot(glyph.horizontalBearing / glyph.size * Vector2f(-1, 1))
				.setSize(glyph.size)
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
