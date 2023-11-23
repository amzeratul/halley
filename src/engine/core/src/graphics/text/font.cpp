#include "halley/graphics/text/font.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"
#include "halley/api/halley_api.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resources.h"
#include "halley/text/string_converter.h"
#include <iostream>

using namespace Halley;

Font::Glyph::Glyph() {}

Font::Glyph::Glyph(int charcode, Rect4f area, Vector2f size, Vector2f horizontalBearing, Vector2f verticalBearing, Vector2f advance)
	: charcode(charcode)
	, area(area)
	, size(size)
	, horizontalBearing(horizontalBearing)
	, verticalBearing(verticalBearing)
	, advance(advance)
{
}

void Font::Glyph::serialize(Serializer& s) const
{
	s << area;
	s << size;
	s << horizontalBearing;
	s << verticalBearing;
	s << advance;
}

void Font::Glyph::deserialize(Deserializer& s)
{
	s >> area;
	s >> size;
	s >> horizontalBearing;
	s >> verticalBearing;
	s >> advance;
}

Font::Font(String name, String imageName, float ascender, float height, float sizePt, float renderScale, Vector2i imageSize)
	: name(std::move(name))
	, imageName(std::move(imageName))
	, ascender(ascender)
	, height(height)
	, sizePt(sizePt)
	, smoothRadius(0)
	, replacementScale(renderScale)
	, imageSize(imageSize)
	, distanceField(false)
	, floorGlyphPosition(false)
{
}

Font::Font(String name, String imageName, float ascender, float height, float sizePt, float renderScale, Vector2i imageSize, float distanceFieldSmoothRadius, Vector<String> fallback, bool floorGlyphPosition)
	: name(std::move(name))
	, imageName(std::move(imageName))
	, ascender(ascender)
	, height(height)
	, sizePt(sizePt)
	, smoothRadius(distanceFieldSmoothRadius)
	, replacementScale(renderScale)
	, imageSize(imageSize)
	, distanceField(true)
	, fallback(std::move(fallback))
	, floorGlyphPosition(floorGlyphPosition)
{
}

std::unique_ptr<Font> Font::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic(false);
	if (!data) {
		return {};
	}

	auto font = std::make_unique<Font>();
	auto ds = Deserializer(data->getSpan());
	font->deserialize(ds);

	auto texture = loader.getResources().get<Texture>(font->imageName);
	auto matDef = loader.getResources().get<MaterialDefinition>(font->distanceField ? "Halley/Text" : MaterialDefinition::defaultMaterial);
	font->material = std::make_unique<Material>(matDef);
	font->material->set(0, texture);

	return font;
}

void Font::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<Font&>(resource));
}

void Font::onLoaded(Resources& resources)
{
	for (auto& fontName: fallback) {
		fallbackFont.push_back(resources.get<Font>(fontName));
	}
}

std::pair<const Font::Glyph&, const Font&> Font::getGlyph(int code) const
{
	const auto& font = getFontForGlyph(code);
	return { font.getGlyphHere(code), font };
}

const Font::Glyph& Font::getGlyphHere(int code) const
{
	auto iter = glyphs.find(code);
	if (iter == glyphs.end()) {
		iter = glyphs.find(0);
		if (iter == glyphs.end()) {
			throw Exception("Unable to load fallback character, needed for character " + toString(code), HalleyExceptions::Graphics);
		}
		return iter->second;
	}
	return iter->second;
}

const Font& Font::getFontForGlyph(int code) const
{
	const auto iter = glyphs.find(code);
	if (iter == glyphs.end()) {
		for (const auto& font: fallbackFont) {
			if (fallbackIgnore.contains(font->getName())) {
				continue;
			}
			if (font->glyphs.find(code) != font->glyphs.end()) {
				return *font;
			}
		}
	}
	return *this;
}

float Font::getLineHeightAtSize(float size) const
{
	return height * size / sizePt;
}

float Font::getAscenderDistance() const
{
	return ascender;
}

float Font::getHeight() const
{
	return height;
}

float Font::getSizePoints() const
{
	return sizePt;
}

float Font::getSmoothRadius() const
{
	return smoothRadius;
}

Vector2i Font::getImageSize() const
{
	return imageSize;
}

float Font::getReplacementScale() const
{
	return replacementScale;
}

String Font::getName() const
{
	return name;
}

bool Font::isDistanceField() const
{
	return distanceField;
}

bool Font::shouldFloorGlyphPosition() const
{
	return floorGlyphPosition;	
}

void Font::addGlyph(const Glyph& glyph)
{
	glyphs[glyph.charcode] = glyph;
}

std::shared_ptr<Material> Font::getMaterial() const
{
	return material;
}

void Font::serialize(Serializer& s) const
{
	s << name;
	s << imageName;
	s << ascender;
	s << height;
	s << sizePt;
	s << distanceField;
	s << smoothRadius;
	s << imageSize;
	s << replacementScale;
	s << glyphs;
	s << fallback;
	s << floorGlyphPosition;
}

void Font::deserialize(Deserializer& s)
{
	s >> name;
	s >> imageName;
	s >> ascender;
	s >> height;
	s >> sizePt;
	s >> distanceField;
	s >> smoothRadius;
	s >> imageSize;
	s >> replacementScale;
	s >> glyphs;
	s >> fallback;
	s >> floorGlyphPosition;

	for (auto& g: glyphs) {
		g.second.charcode = g.first;
	}

	//printGlyphs();
}

void Font::printGlyphs() const
{
	std::optional<Range<int>> curRange;
	Vector<Range<int>> ranges;
	for (auto& g: glyphs) {
		int c = g.first;
		if (curRange && curRange->end == c - 1) {
			curRange->end = c;
		} else {
			if (curRange) {
				ranges.push_back(*curRange);
			}
			curRange = Range<int>(c, c);
		}
	}
	if (curRange) {
		ranges.push_back(*curRange);
	}
	std::cout << name << ": [";
	for (auto& r: ranges) {
		if (r.start != r.end) {
			std::cout << "[" << r.start << ", " << r.end << "], ";
		} else {
			std::cout << "[" << r.start << "], ";
		}
	}
	std::cout << "]\n";
}

HashSet<String> Font::fallbackIgnore;

void Font::setFallbackFontsIgnore(const HashSet<String>&& ignore)
{
	fallbackIgnore = ignore;
}
