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

Font::Font(String name, String imageName, float ascender, float height, float sizePt)
	: name(name)
	, imageName(imageName)
	, ascender(ascender)
	, height(height)
	, sizePt(sizePt)
	, smoothRadius(0)
	, distanceField(false)
{
}

Font::Font(String name, String imageName, float ascender, float height, float sizePt, float distanceFieldSmoothRadius)
	: name(name)
	, imageName(imageName)
	, ascender(ascender)
	, height(height)
	, sizePt(sizePt)
	, smoothRadius(distanceFieldSmoothRadius)
	, distanceField(true)
{
}

Font::Font(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	auto ds = Deserializer(data->getSpan());
	deserialize(ds);

	auto texture = loader.getAPI().getResource<Texture>(imageName);
	auto matDef = loader.getAPI().getResource<MaterialDefinition>(distanceField ? "Halley/Text" : "Halley/Sprite");
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

const Font::Glyph& Font::getGlyph(int code) const
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

std::shared_ptr<const Material> Font::getMaterial() const
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
	s << glyphs;
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
	s >> glyphs;

	for (auto& g: glyphs) {
		g.second.charcode = g.first;
	}
}
