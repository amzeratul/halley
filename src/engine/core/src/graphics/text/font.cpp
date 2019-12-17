#include "graphics/text/font.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/api/halley_api.h"
#include "halley/bytes/byte_serializer.h"
#include "resources/resources.h"
#include "halley/text/string_converter.h"

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

Font::Font(String name, String imageName, float ascender, float height, float sizePt, float renderScale)
	: name(name)
	, imageName(imageName)
	, ascender(ascender)
	, height(height)
	, sizePt(sizePt)
	, smoothRadius(0)
	, replacementScale(renderScale)
	, distanceField(false)
{
}

Font::Font(String name, String imageName, float ascender, float height, float sizePt, float renderScale, float distanceFieldSmoothRadius, std::vector<String> fallback)
	: name(name)
	, imageName(imageName)
	, ascender(ascender)
	, height(height)
	, sizePt(sizePt)
	, smoothRadius(distanceFieldSmoothRadius)
	, replacementScale(renderScale)
	, distanceField(true)
	, fallback(std::move(fallback))
{
}

Font::Font(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	auto ds = Deserializer(data->getSpan());
	deserialize(ds);

	auto texture = loader.getResources().get<Texture>(imageName);
	auto matDef = loader.getResources().get<MaterialDefinition>(distanceField ? "Halley/Text" : "Halley/Sprite");
	material = std::make_unique<Material>(matDef);
	material->set("tex0", texture);
}

std::unique_ptr<Font> Font::loadResource(ResourceLoader& loader)
{
	return std::make_unique<Font>(loader);
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

const Font::Glyph& Font::getGlyph(int code) const
{
	auto& font = getFontForGlyph(code);
	auto iter = font.glyphs.find(code);
	if (iter == font.glyphs.end()) {
		iter = font.glyphs.find(0);
		if (iter == font.glyphs.end()) {
			throw Exception("Unable to load fallback character, needed for character " + toString(code), HalleyExceptions::Graphics);
		}
		return iter->second;
	}
	return iter->second;
}

const Font& Font::getFontForGlyph(int code) const
{
	auto iter = glyphs.find(code);
	if (iter == glyphs.end()) {
		for (auto& font: fallbackFont) {
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
	s << replacementScale;
	s << glyphs;
	s << fallback;
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
	s >> replacementScale;
	s >> glyphs;
	s >> fallback;

	for (auto& g: glyphs) {
		g.second.charcode = g.first;
	}

	//printGlyphs();
}

void Font::printGlyphs() const
{
	Maybe<Range<int>> curRange;
	std::vector<Range<int>> ranges;
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
