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
	if (position != pos) {
		position = pos;
		positionDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setFont(std::shared_ptr<const Font> v)
{
	if (font != v) {
		font = v;

		if (font->isDistanceField()) {
			materialDirty = true;
		}
	}

	return *this;
}

TextRenderer& TextRenderer::setText(const String& v)
{
	const auto newText = v.getUTF32();
	if (newText != text) {
		text = newText;
		glyphsDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setText(const StringUTF32& v)
{
	if (v != text) {
		text = v;
		glyphsDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setText(const LocalisedString& v)
{
	const auto newText = v.getString().getUTF32();
	if (newText != text) {
		text = newText;
		glyphsDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setSize(float v)
{
	if (size != v) {
		size = v;
		glyphsDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setColour(Colour v)
{
	if (colour != v) {
		colour = v;
		glyphsDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setOutline(float v)
{
	if (outline != v) {
		outline = v;
		materialDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setAlignment(float v)
{
	if (align != v) {
		align = v;
		glyphsDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setOffset(Vector2f v)
{
	if (offset != v) {
		offset = v;
		glyphsDirty = true;
	}
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
	if (smoothness != s) {
		smoothness = s;
		materialDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setPixelOffset(Vector2f offset)
{
	if (pixelOffset != offset) {
		pixelOffset = offset;
		glyphsDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setColourOverride(const std::vector<ColourOverride>& colOverride)
{
	if (colourOverrides != colOverride) {
		colourOverrides = colOverride;
		glyphsDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setLineSpacing(float spacing)
{
	if (lineSpacing != spacing) {
		lineSpacing = spacing;
		glyphsDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setOutlineColour(Colour v)
{
	if (outlineColour != v) {
		outlineColour = v;
		materialDirty = true;
	}
	return *this;
}

TextRenderer TextRenderer::clone() const
{
	return *this;
}

void TextRenderer::generateSprites(std::vector<Sprite>& sprites) const
{
	Expects(font);

	const bool hasMaterialOverride = font->isDistanceField();
	if (hasMaterialOverride && materialDirty) {
		updateMaterials();
		materialDirty = false;
	}

	if (glyphsDirty || positionDirty) {
		float scale = getScale();
		Vector2f p = (position + Vector2f(0, font->getAscenderDistance() * scale)).floor();
		if (offset != Vector2f(0, 0)) {
			p -= (getExtents() * offset).floor();
		}

		size_t startPos = 0;
		size_t spritesInserted = 0;
		Vector2f lineOffset;

		auto flush = [&] ()
		{
			// Line break, update previous characters!
			if (align != 0) {
				Vector2f off = (-lineOffset * align).floor();
				for (size_t j = startPos; j < spritesInserted; j++) {
					auto& sprite = sprites[j];
					sprite.setPos(sprite.getPosition() + off);
				}
			}

			// Move pen
			p.y += getLineHeight();

			// Reset
			startPos = spritesInserted;
			lineOffset.x = 0;
		};

		auto curCol = colour;
		size_t curOverride = 0;

		const size_t n = text.size();

		size_t nGlyphs = 0;
		for (size_t i = 0; i < n; i++) {
			if (text[i] != '\n') {
				++nGlyphs;
			}
		}
		sprites.resize(nGlyphs);

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
				auto& fontForGlyph = font->getFontForGlyph(c);
				auto& glyph = fontForGlyph.getGlyph(c);

				std::shared_ptr<Material> materialToUse = hasMaterialOverride ? getMaterial(fontForGlyph) : fontForGlyph.getMaterial();

				sprites[spritesInserted++] = Sprite()
					.setMaterial(materialToUse)
					.setSize(glyph.size)
					.setTexRect(glyph.area)
					.setColour(curCol)
					.setPivot(glyph.horizontalBearing / glyph.size * Vector2f(-1, 1))
					.setScale(scale)
					.setPos(p + lineOffset + pixelOffset);

				lineOffset.x += glyph.advance.x * scale;

				if (i == n - 1) {
					flush();
				}
			}
		}

		glyphsDirty = false;
		positionDirty = false;
	}
}

void TextRenderer::draw(Painter& painter) const
{
	generateSprites(spritesCache);

	if (spriteFilter) {
		// We don't know what the user will do with glyphs, so mark them as dirty
		spriteFilter(gsl::span<Sprite>(spritesCache.data(), spritesCache.size()));
		glyphsDirty = true;
		positionDirty = true;
	}

	if (clip) {
		painter.setRelativeClip(clip.get() + position);
	}
	Sprite::drawMixedMaterials(spritesCache.data(), spritesCache.size(), painter);
	if (clip) {
		painter.setClip();
	}
}

void TextRenderer::setSpriteFilter(SpriteFilter f)
{
	spriteFilter = std::move(f);
}

Vector2f TextRenderer::getExtents() const
{
	return getExtents(text);
}

Vector2f TextRenderer::getExtents(const StringUTF32& str) const
{
	Vector2f p;
	float w = 0;
	const float scale = getScale();
	const float lineH = getLineHeight();

	for (auto& c : str) {
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

Vector2f TextRenderer::getCharacterPosition(size_t character) const
{
	return getCharacterPosition(character, text);
}

Vector2f TextRenderer::getCharacterPosition(size_t character, const StringUTF32& str) const
{
	Vector2f p;
	const float scale = getScale();
	const float lineH = getLineHeight();

	for (size_t i = 0; i < character && i < str.size(); ++i) {
		auto c = str[i];
		if (c == '\n') {
			// Line break!
			p.x = 0;
			p.y += lineH;
		} else {
			auto& glyph = font->getGlyph(c);
			p += Vector2f(glyph.advance.x, 0) * scale;
		}
	}

	return p;
}

size_t TextRenderer::getCharacterAt(const Vector2f& position) const
{
	return getCharacterAt(position, text);
}

size_t TextRenderer::getCharacterAt(const Vector2f& position, const StringUTF32& str) const
{
	const float scale = getScale();
	const float lineH = getLineHeight();
	const int targetLine = int(floor(position.y / lineH));
	Vector2f p;
	int curLine = 0;
	float bestDist2 = std::numeric_limits<float>::max();
	bool gotLineMatch = false;
	size_t bestAnswer = 0;
	const size_t nChars = str.size();

	for (size_t i = 0; i <= nChars; ++i) {
		auto c = i == nChars ? 0 : str[i]; // Add a sigil at the end

		const float dist2 = (p - position).squaredLength();
		if (!gotLineMatch && curLine == targetLine) {
			gotLineMatch = true;
			bestDist2 = dist2;
			bestAnswer = i;
		}
		if (curLine == targetLine || !gotLineMatch) {
			if (dist2 < bestDist2) {
				bestDist2 = dist2;
				bestAnswer = i;
			}
		}

		if (c == '\n') {
			// Line break!
			curLine++;
			if (curLine > targetLine) {
				break;
			}
			p.x = 0;
			p.y += lineH;
		} else if (c != 0) {
			auto& glyph = font->getGlyph(c);
			p += Vector2f(glyph.advance.x, 0) * scale;
		}
	}

	return bestAnswer;
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

	const float scale = getScale();

	gsl::span<const char32_t> src = str;

	// Keep doing this while src is not exhausted
	while (!src.empty()) {
		float curWidth = 0.0f;
		Maybe<gsl::span<const char32_t>> lastValid;

		for (std::ptrdiff_t i = 0; i < src.length(); ++i) {
			const int32_t c = src[i];
			const bool isLastChar = i == src.length() - 1;
			if (c == '\n' || c == ' ' || c == '\t' || isLastChar) {
				lastValid = src.subspan(0, i + 1);
			}

			auto& glyph = font->getGlyph(c);
			const float w = glyph.advance.x * scale;
			curWidth += w;

			if (c == '\n' || curWidth > maxWidth || isLastChar) {
				int advanceAdjust = isLastChar ? 0 : -1;
				if (!lastValid) {
					// lastValid won't be set if there were no suitable breaking spaces
					if (isLastChar) {
						lastValid = src.subspan(0, i + 1);
					} else {
						lastValid = src.subspan(0, i);
					}
					advanceAdjust = 0;
				}
				const int advance = int(lastValid.get().size());
				Expects(advance > 0);

				if (!result.empty()) {
					result.push_back('\n');
				}
				result += StringUTF32(lastValid.get().data(), advance + advanceAdjust);
				src = src.subspan(advance);
				break;
			}
		}
	}
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
	return roundf(font->getHeight() * getScale() + lineSpacing);
}

float TextRenderer::getAlignment() const
{
	return align;
}

float TextRenderer::getScale() const
{
	return size / font->getSizePoints();
}

std::shared_ptr<Material> TextRenderer::getMaterial(const Font& font) const
{
	const auto iter = materials.find(&font);
	if (iter == materials.end()) {
		auto material = font.getMaterial()->clone();
		materials[&font] = material;
		updateMaterial(*material, font);
		return material;
	} else {
		return iter->second;
	}
}

void TextRenderer::updateMaterial(Material& material, const Font& font) const
{
	float smooth = clamp(smoothness / font.getSmoothRadius(), 0.001f, 0.999f);
	float outlineSize = clamp(outline / font.getSmoothRadius(), 0.0f, 0.995f);
	material
		.set("u_smoothness", smooth)
		.set("u_outline", outlineSize)
		.set("u_outlineColour", outlineColour);

	material.setPassEnabled(0, outline > 0.0001f);
}

void TextRenderer::updateMaterialForFont(const Font& font) const
{
	auto material = getMaterial(font);
	updateMaterial(*material, font);
}

void TextRenderer::updateMaterials() const
{
	for (auto& m: materials) {
		updateMaterial(*m.second, *m.first);
	}
}
