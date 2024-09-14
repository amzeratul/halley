#include "halley/graphics/text/text_renderer.h"
#include "halley/graphics/text/font.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_parameter.h"
#include <gsl/assert>

#include "halley/text/i18n.h"

using namespace Halley;

TextRenderer::TextRenderer()
{
}

TextRenderer::TextRenderer(std::shared_ptr<const Font> font, const String& text, float size, Colour colour, float outline, Colour outlineColour)
	: text(text.getUTF32())
	, size(size)
	, outline(outline)
	, colour(colour)
	, outlineColour(outlineColour)
{
	setFont(std::move(font));
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
		font = std::move(v);
		markLayoutDirty();

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
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setText(const StringUTF32& v)
{
	if (v != text) {
		text = v;
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setText(const LocalisedString& v)
{
	const auto newText = v.getString().getUTF32();
	if (newText != text) {
		text = newText;
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setSize(float v)
{
	if (size != v) {
		size = v;
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setColour(Colour v)
{
	if (colour != v) {
		colour = v;
		markSpritesDirty();
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
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setOffset(Vector2f v)
{
	if (offset != v) {
		offset = v;
		markLayoutDirty();
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
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setColourOverride(Vector<ColourOverride> colOverride)
{
	if (colourOverrides != colOverride) {
		colourOverrides = std::move(colOverride);
		markSpritesDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setFontOverride(Vector<FontOverride> fontOverride)
{
	if (fontOverrides != fontOverride) {
		fontOverrides = std::move(fontOverride);
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setFontSizeOverride(Vector<FontSizeOverride> fontSizeOverride)
{
	if (fontSizeOverrides != fontSizeOverride) {
		fontSizeOverrides = std::move(fontSizeOverride);
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setLineSpacing(float spacing)
{
	if (lineSpacing != spacing) {
		lineSpacing = spacing;
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setAlpha(float alpha)
{
	const auto c = colour.withAlpha(alpha);
	if (colour != c) {
		colour = c;
		markSpritesDirty();
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
		markLayoutDirty();
	}
	return *this;
}

TextRenderer& TextRenderer::setAngle(Angle1f angle)
{
	if (this->angle != angle) {
		this->angle = angle;
		markLayoutDirty();
	}
	return *this;
}

void TextRenderer::refresh()
{
	markLayoutDirty();
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

void TextRenderer::generateLayoutIfNeeded() const
{
	if (!font) {
		return;
	}

	if (layoutDirty || positionDirty) {
		generateLayout(text, &positionsCache, extents);
		hasExtents = true;
		positionDirty = false;
		layoutDirty = false;
		glyphsDirty = true;
	}
}

void TextRenderer::generateGlyphsIfNeeded() const
{
	if (!font) {
		return;
	}

	const bool hasMaterialOverride = font->isDistanceField();
	if (hasMaterialOverride && materialDirty) {
		updateMaterials();
		materialDirty = false;
	}

	if (glyphsDirty) {
		generateSprites(spritesCache, positionsCache);
		glyphsDirty = false;
	}
}

void TextRenderer::generateSprites() const
{
	generateLayoutIfNeeded();
	generateGlyphsIfNeeded();
}

void TextRenderer::generateLayout(const StringUTF32& text, Vector<Vector2f>* positions, Vector2f& extents) const
{
	const bool floorEnabled = font->shouldFloorGlyphPosition();
	const auto floorAlign = [floorEnabled] (Vector2f a) { return floorEnabled ? a.floor() : a; };

	Vector2f lineStartPos;

	size_t firstSpriteInCurrentLine = 0;
	size_t spritesInserted = 0;
	Vector2f curLineOffset;
	float curLineHeight = 0;
	float curAscender = 0;

	if (positions) {
		positions->resize(getGlyphCount(text));
	}

	const Font::Glyph* lastGlyph = nullptr;
	const Font* lastFont = nullptr;

	auto curFont = TextOverrideCursor(font, fontOverrides);
	auto curFontSize = TextOverrideCursor(size, fontSizeOverrides);

	float minX = std::numeric_limits<float>::infinity();
	float maxX = -std::numeric_limits<float>::infinity();
	float height = 0;
	bool gotCharacter = false;

	// Go through every character
	const size_t n = text.size();
	for (size_t i = 0; i < n; i++) {
		const char32_t c = text[i];
		curFont.setPos(i);
		curFontSize.setPos(i);
		
		if (c != '\n') {
			const auto& [glyph, fontForGlyph] = curFont->getGlyph(c);
			const float curScale = getScale(fontForGlyph, *curFontSize);
			const Vector2f fontAdjustment = floorAlign(Vector2f(0, fontForGlyph.getAscenderDistance() - curFont->getAscenderDistance()) * curScale);

			const Vector2f kerning = lastGlyph && lastFont == (*curFont).get() ? lastGlyph->getKerning(c) : Vector2f();
			const Vector2f glyphPos = lineStartPos + curLineOffset + pixelOffset + fontAdjustment + kerning * curScale + glyph.horizontalBearing * curScale * Vector2f(1, -1);

			if (positions) {
				(*positions)[spritesInserted++] = glyphPos;
			}

			curLineOffset.x += (glyph.advance.x + kerning.x) * curScale;
			curLineHeight = std::max(curLineHeight, getLineHeight(*(*curFont), *curFontSize));
			curAscender = std::max(curAscender, curFont->getAscenderDistance() * curScale);

			minX = std::min(minX, glyphPos.x);
			maxX = std::max(maxX, curLineOffset.x);
			gotCharacter = true;

			lastGlyph = &glyph;
			lastFont = (*curFont).get();
		}

		if (c == '\n' || i == n - 1) {
			// Line break, update previous characters!
			if (positions) {
				const Vector2f lineOffset = floorAlign(position + Vector2f(0, curAscender) - curLineOffset * align);
				for (size_t j = firstSpriteInCurrentLine; j < spritesInserted; j++) {
					(*positions)[j] += lineOffset;
				}
			}

			// Move pen
			lineStartPos.y += curLineHeight;
			height += curLineHeight;
			curLineHeight = 0;
			curAscender = 0;

			// Reset
			firstSpriteInCurrentLine = spritesInserted;
			curLineOffset.x = 0;
		}
	}

	extents = Vector2f(gotCharacter ? (maxX - minX) : 0.0f, height);

	if (positions) {
		if (offset != Vector2f(0, 0)) {
			for (auto& p : *positions) {
				p -= floorAlign(extents * offset);
			}
		}
	}
}

void TextRenderer::generateSprites(Vector<Sprite>& sprites, const Vector<Vector2f>& positions) const
{
	const bool hasMaterialOverride = font->isDistanceField();

	size_t spritesInserted = 0;

	sprites.resize(getGlyphCount(text));
	assert(sprites.size() == positions.size());

	auto curCol = TextOverrideCursor(colour, colourOverrides);
	auto curFont = TextOverrideCursor(font, fontOverrides);
	auto curFontSize = TextOverrideCursor(size, fontSizeOverrides);

	const size_t n = text.size();
	for (size_t i = 0; i < n; i++) {
		const char32_t c = text[i];
		curCol.setPos(i);
		curFont.setPos(i);
		curFontSize.setPos(i);
		
		if (c != '\n') {
			const auto& [glyph, fontForGlyph] = curFont->getGlyph(c);
			const float curScale = getScale(fontForGlyph, *curFontSize);

			auto idx = spritesInserted++;

			const Vector2f glyphPos = positions[idx];
			const Vector2f renderPos = (glyphPos - position).rotate(angle) + position;

			sprites[idx] = Sprite()
				.setMaterial(hasMaterialOverride ? getMaterial(fontForGlyph) : fontForGlyph.getMaterial())
				.setSize(glyph.size)
				.setTexRect(glyph.area)
				.setPos(renderPos)
				.setScale(curScale)
				.setColour(curCol.getCurValue())
				.setRotation(angle);
		}
	}
}

void TextRenderer::draw(Painter& painter, const std::optional<Rect4f>& extClip) const
{
	generateSprites();

	if (spriteFilter) {
		// We don't know what the user will do with glyphs, so mark them as dirty
		spriteFilter(gsl::span<Sprite>(spritesCache.data(), spritesCache.size()));
		markSpritesDirty();
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

	Vector2f result;
	generateLayout(str, nullptr, result);
	return result;
}

Vector2f TextRenderer::getCharacterPosition(size_t character) const
{
	if (text.empty()) {
		return {};
	}
	generateLayoutIfNeeded();

	size_t nLineBreaks = 0;
	for (size_t i = 0; i <= character && i < text.size(); ++i) {
		if (text[i] == '\n') {
			++nLineBreaks;
		}
	}

	size_t idx = character - std::min(character, nLineBreaks);
	if (idx >= positionsCache.size()) {
		return positionsCache.back();
	}
	return positionsCache[idx];
}

size_t TextRenderer::getCharacterAt(const Vector2f& targetPos) const
{
	generateLayoutIfNeeded();

	float bestDist2 = std::numeric_limits<float>::max();
	size_t bestResult = 0;

	for (size_t i = 0; i < text.size(); ++i) {
		const auto pos = positionsCache[i];
		const auto delta = (targetPos - pos);
		if (delta.y < 0) {
			return bestResult;
		}

		const float dist2 = delta.squaredLength();
		if (dist2 < bestDist2) {
			bestDist2 = dist2;
			bestResult = i;
		}
	}

	return bestResult;
}

StringUTF32 TextRenderer::split(float maxWidth) const
{
	return split(text, maxWidth, {}, fontOverrides, fontSizeOverrides);
}

StringUTF32 TextRenderer::split(const String& str, float maxWidth) const
{
	return split(str.getUTF32(), maxWidth, {}, fontOverrides, fontSizeOverrides);
}

StringUTF32 TextRenderer::split(const StringUTF32& str, float maxWidth, std::function<bool(int32_t)> filter, const Vector<FontOverride>& fontOverrides, const Vector<FontSizeOverride>& fontSizeOverrides) const
{
	StringUTF32 result;

	gsl::span<const char32_t> src = str;
	const Font::Glyph* lastGlyph = nullptr;
	const Font* lastFont = nullptr;

	auto curFont = TextOverrideCursor(font, fontOverrides);
	auto curFontSize = TextOverrideCursor(size, fontSizeOverrides);

	// Keep doing this while src is not exhausted
	while (!src.empty()) {
		float curWidth = 0.0f;
		std::optional<gsl::span<const char32_t>> lastValid;

		for (size_t i = 0; i < src.size(); ++i) {
			const int32_t c = src[i];
			curFont.setPos(i);
			curFontSize.setPos(i);

			const bool accepted = filter ? filter(c) : true;

			const bool isLastChar = i == src.size() - 1;
			if (isLastChar || (accepted && (c == '\n' || c == ' ' || c == '\t'))) {
				lastValid = src.subspan(0, i + 1);
			}

			const auto& [glyph, f] = curFont->getGlyph(c);
			const float scale = getScale(f, *curFontSize);
			const auto kerning = lastFont == (*curFont).get() && lastGlyph ? lastGlyph->getKerning(c) : Vector2f();
			const float w = accepted ? (glyph.advance.x + kerning.x) * scale : 0.0f;
			curWidth += w;

			lastFont = (*curFont).get();
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
	return font ? getLineHeight(*font, size) : 1.0f;
}

float TextRenderer::getLineHeight(const Font& font, float size) const
{
	return roundf(font.getHeight() * getScale(font, size) + lineSpacing);
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

float TextRenderer::getScale(const Font& font) const
{
	return getScale(font, size);
}

float TextRenderer::getScale(const Font& f, float size) const
{
	const bool usingReplacement = &f != font.get();
	return size / f.getSizePoints() * (usingReplacement ? font->getReplacementScale() : 1.0f) * scale;
}

void TextRenderer::markLayoutDirty() const
{
	layoutDirty = true;
	hasExtents = false;
}

void TextRenderer::markSpritesDirty() const
{
	glyphsDirty = true;
}

size_t TextRenderer::getGlyphCount(const StringUTF32& text)
{
	size_t nGlyphs = 0;
	const auto n = text.size();
	for (size_t i = 0; i < n; i++) {
		if (text[i] != '\n') {
			++nGlyphs;
		}
	}
	return nGlyphs;
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
