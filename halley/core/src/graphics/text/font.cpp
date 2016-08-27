#include "graphics/text/font.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/api/halley_api.h"
#include "halley/file_formats/serializer.h"
#include "resources/resources.h"
#include <yaml-cpp/yaml.h>

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

void Font::Glyph::deserialize(Deserializer& s)
{
	// See font_generator.cpp
	s >> area;
	s >> size;
	s >> horizontalBearing;
	s >> verticalBearing;
	s >> advance;
}

Font::Font(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	auto ds = Deserializer(data->getSpan());
	deserialize(ds);

	auto texture = loader.getAPI().getResource<Texture>("../font/" + imageName);
	auto matDef = loader.getAPI().getResource<MaterialDefinition>("text.yaml");
	material = std::make_unique<Material>(matDef);
	(*material)["tex0"] = texture;
}

std::unique_ptr<Font> Font::loadResource(ResourceLoader& loader)
{
	return std::make_unique<Font>(loader);
}

const Font::Glyph& Font::getGlyph(int code) const
{
	auto iter = glyphs.find(code);
	if (iter == glyphs.end()) {
		iter = glyphs.find(0);
		if (iter == glyphs.end()) {
			throw Exception("Unable to load fallback character.");
		}
		return iter->second;
	}
	return iter->second;
}

std::shared_ptr<const Material> Font::getMaterial() const
{
	return material;
}

void Font::deserialize(Deserializer& s)
{
	// See font_generator.cpp
	s >> name;
	s >> imageName;
	s >> ascender;
	s >> height;
	s >> sizePt;
	s >> smoothRadius;
	s >> glyphs;

	for (auto& g: glyphs) {
		g.second.charcode = g.first;
	}
}
