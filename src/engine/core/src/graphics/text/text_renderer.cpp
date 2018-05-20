#include "graphics/text/text_renderer.h"
#include "graphics/text/font.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <gsl/gsl_assert>
#include "halley/text/i18n.h"

using namespace Halley;

TextRenderer::TextRenderer()
{
}

TextRenderer::TextRenderer(std::shared_ptr<const Font> font, String text, float size, Colour colour, float outline, Colour outlineColour)
	: text(text.getUTF32())
	, size(size)
	, outline(outline)
	, colour(colour)
	, outlineColour(outlineColour)
{
	setFont(font);
}

TextRenderer& TextRenderer::setPosition(Vector2f pos)
{
	position = pos;
	return *this;
}

TextRenderer& TextRenderer::setFont(std::shared_ptr<const Font> v)
{
	if (font != v) {
		font = v;
		material = font->getMaterial()->clone();
	}

	return *this;
}

TextRenderer& TextRenderer::setText(const String& v)
{
	text = v.getUTF32();
	return *this;
}

TextRenderer& TextRenderer::setText(const StringUTF32& v)
{
	text = v;
	return *this;
}

TextRenderer& TextRenderer::setText(const LocalisedString& v)
{
	text = v.getString().getUTF32();
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

TextRenderer& TextRenderer::setClip(Rect4f c)
{
	clip = c;
	return *this;
}

TextRenderer& TextRenderer::setClip()
{
	clip.reset();
	return *this;
}

TextRenderer& TextRenderer::setSmoothness(float s)
{
	smoothness = s;
	return *this;
}

TextRenderer& TextRenderer::setPixelOffset(Vector2f offset)
{
	pixelOffset = offset;
	return *this;
}

TextRenderer& TextRenderer::setColourOverride(const std::vector<ColourOverride>& colOverride)
{
	colourOverrides = colOverride;
	return *this;
}

TextRenderer& TextRenderer::setLineSpacing(float spacing)
{
	lineSpacing = spacing;
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
		
	if (font->isDistanceField()) {
		float smooth = clamp(smoothness / font->getSmoothRadius(), 0.001f, 0.999f);
		float outlineSize = clamp(outline / font->getSmoothRadius(), 0.0f, 0.995f);
		material
			->set("u_smoothness", smooth)
			.set("u_outline", outlineSize)
			.set("u_outlineColour", outlineColour);

		material->setPassEnabled(0, outline > 0.0001f);
	}

	float scale = size / font->getSizePoints();
	Vector2f p = (position + Vector2f(0, font->getAscenderDistance() * scale)).floor();
	if (offset != Vector2f(0, 0)) {
		p -= (getExtents() * offset).floor();
	}

	Vector<Sprite> sprites;

	size_t startPos = 0;
	Vector2f lineOffset;

	auto flush = [&] ()
	{
		// Line break, update previous characters!
		if (align != 0) {
			Vector2f off = (-lineOffset * align).floor();
			for (size_t j = startPos; j < sprites.size(); j++) {
				auto& sprite = sprites[j];
				sprite.setPos(sprite.getPosition() + off);
			}
		}

		// Move pen
		p.y += getLineHeight();

		// Reset
		startPos = sprites.size();
		lineOffset.x = 0;
	};

	auto curCol = colour;
	size_t curOverride = 0;

	const size_t n = text.size();
	for (size_t i = 0; i < n; i++) {
		int c = text[i];

		// Check for colour override
		while (curOverride < colourOverrides.size() && colourOverrides[curOverride].first == i) {
			curCol = colourOverrides[curOverride].second ? colourOverrides[curOverride].second.get() : colour;
			++curOverride;
		}
		
		if (c == '\n') {
			flush();
		} else {
			auto& glyph = font->getGlyph(c);

			sprites.push_back(Sprite()
				.setMaterial(material)
				.setSize(glyph.size)
				.setTexRect(glyph.area)
				.setColour(curCol)
				.setPivot(glyph.horizontalBearing / glyph.size * Vector2f(-1, 1))
				.setScale(scale)
				.setPos(p + lineOffset + pixelOffset));

			lineOffset.x += glyph.advance.x * scale;

			if (i == n - 1) {
				flush();
			}
		}
	}

	if (clip) {
		painter.setRelativeClip(clip.get() + position);
	}
	Sprite::draw(sprites.data(), sprites.size(), painter);
	if (clip) {
		painter.setClip();
	}
}

Vector2f TextRenderer::getExtents() const
{
	Vector2f p;
	float w = 0;
	float scale = size / font->getSizePoints();
	float lineH = getLineHeight();

	for (auto& c : text) {
		if (c == '\n') {
			// Line break!
			w = std::max(w, p.x);
			p.x = 0;
			p.y += lineH;
		} else {
			auto& glyph = font->getGlyph(c);
			p += Vector2f(glyph.advance.x, 0) * scale;
		}
	}
	w = std::max(w, p.x);

	return Vector2f(w, p.y + lineH);
}


StringUTF32 TextRenderer::split(float maxWidth) const
{
	return split(text, maxWidth);
}

StringUTF32 TextRenderer::split(const String& str, float maxWidth) const
{
	return split(str.getUTF32(), maxWidth);
}

StringUTF32 TextRenderer::split(const StringUTF32& str, float maxWidth) const
{
	StringUTF32 result;
	if (str.empty()) {
		return result;
	}

	float scale = size / font->getSizePoints();
	float curWidth = 0.0f;
	size_t startPoint = 0;
	size_t lastSplitPoint = 0;

	auto split = [&] (size_t i, bool last = false)
	{
		if (last) {
			result += str.substr(startPoint);
		} else {
			if (lastSplitPoint > startPoint) {
				result += str.substr(startPoint, lastSplitPoint - startPoint);
				startPoint = lastSplitPoint + 1;
			} else {
				lastSplitPoint = i - 1;
				result += str.substr(startPoint, lastSplitPoint - startPoint + 1);
				startPoint = i;
			}
			result += '\n';
			curWidth = 0;
		}
	};

	for (size_t i = 0; i < str.length(); ++i) {
		auto c = str[i];
		if (c == '\n') {
			lastSplitPoint = i;
			split(i);
		} else {
			if (c == ' ' || c == '\t') {
				lastSplitPoint = i;
			}
			auto& glyph = font->getGlyph(c);
			float w = glyph.advance.x * scale;
			if (curWidth + w < maxWidth) {
				curWidth += w;
			} else {
				split(i);
				i = lastSplitPoint;
			}
		}
	}
	split(str.length() - 1, true);

	return result;
}

Vector2f TextRenderer::getPosition() const
{
	return position;
}

String TextRenderer::getText() const
{
	return String(text);
}

Colour TextRenderer::getColour() const
{
	return colour;
}

Colour TextRenderer::getOutlineColour() const
{
	return outlineColour;
}

float TextRenderer::getSmoothness() const
{
	return smoothness;
}

Maybe<Rect4f> TextRenderer::getClip() const
{
	return clip;
}

float TextRenderer::getLineHeight() const
{
	float scale = size / font->getSizePoints();
	return roundf(font->getHeight() * scale + lineSpacing);
}
