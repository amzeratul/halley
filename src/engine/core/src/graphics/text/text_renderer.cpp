#include "halley/graphics/text/text_renderer.h"
#include "halley/graphics/text/font.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_parameter.h"
#include <gsl/assert>

#include "halley/support/logger.h"
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
		markGlyphsDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setText(const StringUTF32& v)
{
	if (v != text) {
		text = v;
		markGlyphsDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setText(const LocalisedString& v)
{
	const auto newText = v.getString().getUTF32();
	if (newText != text) {
		text = newText;
		markGlyphsDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setSize(float v)
{
	if (size != v) {
		size = v;
		markGlyphsDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setColour(Colour v)
{
	if (colour != v) {
		colour = v;
		markGlyphsDirty();
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

TextRenderer& TextRenderer::setOutline(float width, Colour colour)
{
	if (outline != width || outlineColour != colour) {
		outline = width;
		outlineColour = colour;
		materialDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setShadow(float distance, float smoothness, Colour colour)
{
	return setShadow(Vector2f(distance, distance), smoothness, colour);
}

TextRenderer& TextRenderer::setShadow(Vector2f distance, float smoothness, Colour colour)
{
	if (shadowDistance != distance || shadowSmoothness != smoothness || shadowColour != colour) {
		shadowDistance = distance;
		shadowSmoothness = smoothness;
		shadowColour = colour;
		materialDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setShadowColour(Colour colour)
{
	if (shadowColour != colour) {
		shadowColour = colour;
		materialDirty = true;
	}
	return *this;
}

TextRenderer& TextRenderer::setAlignment(float v)
{
	if (align != v) {
		align = v;
		markGlyphsDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setOffset(Vector2f v)
{
	if (offset != v) {
		offset = v;
		markGlyphsDirty();
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
		markGlyphsDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setColourOverride(Vector<ColourOverride> colOverride)
{
	if (colourOverrides != colOverride) {
		colourOverrides = std::move(colOverride);
		markGlyphsDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setLineSpacing(float spacing)
{
	if (lineSpacing != spacing) {
		lineSpacing = spacing;
		markGlyphsDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setAlpha(float alpha)
{
	const auto c = colour.withAlpha(alpha);
	if (colour != c) {
		colour = c;
		markGlyphsDirty();
	}

	const auto oc = outlineColour.withAlpha(alpha);
	if (outlineColour != oc) {
		outlineColour = oc;
		materialDirty = true;
	}

	return *this;
}

TextRenderer& TextRenderer::setScale(float scale)
{
	if (this->scale != scale) {
		this->scale = scale;
		markGlyphsDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setAngle(Angle1f angle)
{
	if (this->angle != angle) {
		this->angle = angle;
		markGlyphsDirty();
	}
	return *this;
}

void TextRenderer::refresh()
{
	markGlyphsDirty();
	materialDirty = true;
	positionDirty = true;
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

void TextRenderer::generateSprites() const
{
	generateSprites(spritesCache);
}

void TextRenderer::generateSprites(Vector<Sprite>& sprites) const
{
	if (!font) {
		return;
	}

	bool floorEnabled = font->shouldFloorGlyphPosition();
	auto floorAlign = [floorEnabled] (Vector2f a) -> Vector2f
	{
		if (floorEnabled) {
			return a.floor();
		} else {
			return a;
		}
	};

	const bool hasMaterialOverride = font->isDistanceField();
	if (hasMaterialOverride && materialDirty) {
		updateMaterials();
		materialDirty = false;
	}

	if (glyphsDirty || positionDirty) {
		float mainScale = getScale(*font);
		Vector2f p = floorAlign(position + Vector2f(0, font->getAscenderDistance() * mainScale));
		if (offset != Vector2f(0, 0)) {
			p -= floorAlign(getExtents() * offset);
		}

		size_t startPos = 0;
		size_t spritesInserted = 0;
		Vector2f lineOffset;

		auto flush = [&] ()
		{
			// Line break, update previous characters!
			if (align != 0) {
				Vector2f off = floorAlign(-lineOffset * align).rotate(angle);
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

		const Font::Glyph* lastGlyph = nullptr;
		const Font* lastFont = nullptr;

		for (size_t i = 0; i < n; i++) {
			int c = text[i];

			// Check for colour override
			while (curOverride < colourOverrides.size() && colourOverrides[curOverride].first == i) {
				curCol = colourOverrides[curOverride].second ? colourOverrides[curOverride].second.value() : colour;
				++curOverride;
			}
			
			if (c == '\n') {
				flush();
			} else {
				const auto& [glyph, fontForGlyph] = font->getGlyph(c);
				const float scale = getScale(fontForGlyph);
				const auto fontAdjustment = floorAlign(Vector2f(0, fontForGlyph.getAscenderDistance() - font->getAscenderDistance()) * scale);

				const auto kerning = lastGlyph && lastFont == font.get() ? lastGlyph->getKerning(c) : Vector2f();
				const auto glyphPos = p + lineOffset + pixelOffset + fontAdjustment + kerning * scale;
				const auto renderPos = (glyphPos - position).rotate(angle) + position;

				std::shared_ptr<Material> materialToUse = hasMaterialOverride ? getMaterial(fontForGlyph) : fontForGlyph.getMaterial();

				sprites[spritesInserted++] = Sprite()
					.setMaterial(std::move(materialToUse))
					.setSize(glyph.size)
					.setTexRect(glyph.area)
					.setColour(curCol)
					.setPivot(glyph.horizontalBearing / glyph.size * Vector2f(-1, 1))
					.setScale(scale)
					.setPos(renderPos)
					.setRotation(angle);

				lineOffset.x += (glyph.advance.x + kerning.x) * scale;

				lastGlyph = &glyph;
				lastFont = font.get();

				if (i == n - 1) {
					flush();
				}
			}
		}

		glyphsDirty = false;
		positionDirty = false;
	}
}

void TextRenderer::draw(Painter& painter, const std::optional<Rect4f>& extClip) const
{
	generateSprites(spritesCache);

	if (spriteFilter) {
		// We don't know what the user will do with glyphs, so mark them as dirty
		spriteFilter(gsl::span<Sprite>(spritesCache.data(), spritesCache.size()));
		markGlyphsDirty();
		positionDirty = true;
	}

	const std::optional<Rect4f> myClip = clip ? clip.value() + position : std::optional<Rect4f>();
	const auto finalClip = Rect4f::optionalIntersect(myClip, extClip);
	if (finalClip) {
		painter.setRelativeClip(finalClip.value());
	}
	
	Sprite::drawMixedMaterials(spritesCache.data(), spritesCache.size(), painter);

	if (finalClip) {
		painter.setClip();
	}

	//painter.drawRect(getAABB(), 0.5f, Colour4f(0, 1, 0));
}

void TextRenderer::setSpriteFilter(SpriteFilter f)
{
	spriteFilter = std::move(f);
}

Vector2f TextRenderer::getExtents() const
{
	if (hasExtents) {
		return extents;
	}

	if (!font) {
		hasExtents = true;
		extents = {};
		return {};
	}
	
	extents = getExtents(text);
	hasExtents = true;
	return extents;
}

Vector2f TextRenderer::getExtents(const StringUTF32& str) const
{
	if (!font) {
		return {};
	}

	Vector2f p;
	float w = 0;
	const float lineH = getLineHeight();

	const Font* lastFont = nullptr;
	const Font::Glyph* lastGlyph = nullptr;

	for (auto& c : str) {
		if (c == '\n') {
			// Line break!
			w = std::max(w, p.x);
			p.x = 0;
			p.y += lineH;
		} else {
			const auto& [glyph, f] = font->getGlyph(c);
			const float scale = getScale(f);

			const auto kerning = lastGlyph && lastFont == font.get() ? lastGlyph->getKerning(c) : Vector2f();

			p += Vector2f(glyph.advance.x + kerning.x, 0) * scale;

			lastGlyph = &glyph;
			lastFont = font.get();
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
	const float lineH = getLineHeight();

	const Font* lastFont = nullptr;
	const Font::Glyph* lastGlyph = nullptr;

	for (size_t i = 0; i < character && i < str.size(); ++i) {
		auto c = str[i];
		if (c == '\n') {
			// Line break!
			p.x = 0;
			p.y += lineH;
		} else {
			const auto& [glyph, f] = font->getGlyph(c);
			const float scale = getScale(f);
			const auto kerning = lastGlyph && lastFont == font.get() ? lastGlyph->getKerning(c) : Vector2f();
			p += Vector2f(glyph.advance.x + kerning.x, 0) * scale;

			lastGlyph = &glyph;
			lastFont = font.get();		}
	}

	return p;
}

size_t TextRenderer::getCharacterAt(const Vector2f& position) const
{
	return getCharacterAt(position, text);
}

size_t TextRenderer::getCharacterAt(const Vector2f& position, const StringUTF32& str) const
{
	const float lineH = getLineHeight();
	const int targetLine = int(floor(position.y / lineH));
	Vector2f p;
	int curLine = 0;
	float bestDist2 = std::numeric_limits<float>::max();
	bool gotLineMatch = false;
	size_t bestAnswer = 0;
	const size_t nChars = str.size();
	const Font* lastFont = nullptr;
	const Font::Glyph* lastGlyph = nullptr;

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
			const auto& [glyph, f] = font->getGlyph(c);
			const float scale = getScale(f);
			const auto kerning = lastGlyph && lastFont == font.get() ? lastGlyph->getKerning(c) : Vector2f();
			p += Vector2f(glyph.advance.x + kerning.x, 0) * scale;

			lastGlyph = &glyph;
			lastFont = font.get();		}
	}

	if (!gotLineMatch) {
		return targetLine < 0 ? 0 : nChars;
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

StringUTF32 TextRenderer::split(const StringUTF32& str, float maxWidth, std::function<bool(int32_t)> filter) const
{
	StringUTF32 result;

	gsl::span<const char32_t> src = str;
	const Font::Glyph* lastGlyph = nullptr;
	const Font* lastFont = nullptr;

	// Keep doing this while src is not exhausted
	while (!src.empty()) {
		float curWidth = 0.0f;
		std::optional<gsl::span<const char32_t>> lastValid;

		for (size_t i = 0; i < src.size(); ++i) {
			const int32_t c = src[i];
			const bool accepted = filter ? filter(c) : true;

			const bool isLastChar = i == src.size() - 1;
			if (isLastChar || (accepted && (c == '\n' || c == ' ' || c == '\t'))) {
				lastValid = src.subspan(0, i + 1);
			}

			const auto& [glyph, f] = font->getGlyph(c);
			const float scale = getScale(f);
			const auto kerning = lastFont == font.get() && lastGlyph ? lastGlyph->getKerning(c) : Vector2f();
			const float w = accepted ? (glyph.advance.x + kerning.x) * scale : 0.0f;
			curWidth += w;

			lastFont = font.get();
			lastGlyph = &glyph;

			const bool firstCharInRun = i == 0; // It MUST fit at least the first character, or we'll infinite loop
			if (c == '\n' || (!firstCharInRun && curWidth > maxWidth) || isLastChar) {
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
				const int advance = int(lastValid->size());

				if (!result.empty()) {
					result.push_back('\n');
				}
				const int totalAdvance = std::max(advance + advanceAdjust, 0);
				result += StringUTF32(lastValid->data(), totalAdvance);
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

const StringUTF32& TextRenderer::getTextUTF32() const
{
	return text;
}

Colour TextRenderer::getColour() const
{
	return colour;
}

float TextRenderer::getOutline() const
{
	return outline;
}

Colour TextRenderer::getOutlineColour() const
{
	return outlineColour;
}

Colour TextRenderer::getShadowColour() const
{
	return shadowColour;
}

float TextRenderer::getSmoothness() const
{
	return smoothness;
}

std::optional<Rect4f> TextRenderer::getClip() const
{
	return clip;
}

float TextRenderer::getLineHeight() const
{
	return font ? roundf(font->getHeight() * getScale(*font) + lineSpacing) : 1.0f;
}

float TextRenderer::getAlignment() const
{
	return align;
}

bool TextRenderer::empty() const
{
	return text.empty();
}

Rect4f TextRenderer::getAABB() const
{
	const auto pos = getPosition();
	const auto size = getExtents();
	return pos + Rect4f(Vector2f(), size) - size * offset;
}

bool TextRenderer::isCompatibleWith(const TextRenderer& other) const
{
	if (!font || !other.font) {
		// At least one won't draw, so no incompatibility
		return true;
	}
	return getMaterial(*font)->isCompatibleWith(*other.getMaterial(*other.font));
}

float TextRenderer::getScale(const Font& f) const
{
	const bool usingReplacement = &f != font.get();
	return size / f.getSizePoints() * (usingReplacement ? font->getReplacementScale() : 1.0f) * scale;
}

void TextRenderer::markGlyphsDirty() const
{
	glyphsDirty = true;
	hasExtents = false;
}

const std::shared_ptr<Material>& TextRenderer::getMaterial(const Font& font) const
{
	if (const auto iter = materials.find(&font); iter != materials.end()) {
		return iter->second;
	} else {
		auto material = font.getMaterial()->clone();
		updateMaterial(*material, font);
		materials[&font] = std::move(material);
		return materials.at(&font);
	}
}

void TextRenderer::updateMaterial(Material& material, const Font& font) const
{
	const float smoothRadius = font.getSmoothRadius();
	const Vector2f smoothPerTexUnit = Vector2f(font.getImageSize()) / (2.0f * smoothRadius);
	const float lowSmooth = std::min(smoothPerTexUnit.x, smoothPerTexUnit.y);

	const float fontScale = getScale(font);
	
	const float smooth = smoothness * lowSmooth;
	const float shadowSmooth = shadowSmoothness * lowSmooth;
	const float outlineSize = outline * scale / smoothRadius / fontScale;

	material
		.set("u_smoothness", smooth)
		.set("u_outline", outlineSize)
		.set("u_shadowDistance", shadowDistance)
		.set("u_shadowSmoothness", shadowSmooth)
		.set("u_outlineColour", outlineColour)
		.set("u_shadowColour", shadowColour);

	material.setPassEnabled(0, shadowColour.a > 0.001f);
	material.setPassEnabled(1, outlineColour.a > 0.001f && outline > 0.0001f);
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

ColourStringBuilder::ColourStringBuilder(bool replaceEmptyWithQuotes)
	: replaceEmptyWithQuotes(replaceEmptyWithQuotes)
{
}

void ColourStringBuilder::append(std::string_view text, std::optional<Colour4f> col)
{
	if ((colours.empty() && col) || (!colours.empty() && col != colours.back().second)) {
		colours.emplace_back(len, col);	
	}
	if (text.empty() && replaceEmptyWithQuotes) {
		len += 2;
		strings.push_back("\"\"");
	} else {
		len += String::getUTF32Len(text);
		strings.emplace_back(text);
	}
}

std::pair<String, Vector<ColourOverride>> ColourStringBuilder::moveResults()
{
	String result;
	result.cppStr().reserve(len + 1);
	for (auto& s: strings) {
		result += s;
	}
	strings.clear();
	len = 0;
	return { std::move(result), std::move(colours) };
}

ConfigNode ConfigNodeSerializer<TextRenderer>::serialize(const TextRenderer& text, const EntitySerializationContext& context)
{
	// TODO
	ConfigNode node;
	return node;
}

void ConfigNodeSerializer<TextRenderer>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, TextRenderer& target)
{
	target.setFont(context.resources->get<Font>(node["font"].asString("Ubuntu Bold")));

	if (node.hasKey("text")) {
		target.setText(node["text"].asString());
	}
	if (node.hasKey("size")) {
		target.setSize(node["size"].asFloat());
	}
	if (node.hasKey("outline")) {
		target.setOutline(node["outline"].asFloat());
	}
	if (node.hasKey("colour")) {
		target.setColour(Colour4f::fromString(node["colour"].asString()));
	}
	if (node.hasKey("outlineColour")) {
		target.setOutlineColour(Colour4f::fromString(node["outlineColour"].asString()));
	}
	if (node.hasKey("shadowColour")) {
		target.setShadow(node["shadowDistance"].asVector2f({}), node["shadowSmoothness"].asFloat(1), Colour4f::fromString(node["shadowColour"].asString()));
	}
	if (node.hasKey("alignment")) {
		target.setAlignment(node["alignment"].asFloat());
	}
	if (node.hasKey("offset")) {
		target.setOffset(node["offset"].asVector2f());
	}
	if (node.hasKey("smoothness")) {
		target.setSmoothness(node["smoothness"].asFloat());
	}
}
