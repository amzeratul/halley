#include "font.h"
#include "../material/material.h"
#include "../material/material_definition.h"
#include "../material/material_parameter.h"
#include "../../api/halley_api.h"
#include "../../resources/resources.h"
#include <yaml-cpp/yaml.h>

using namespace Halley;

Font::Glyph::Glyph(Glyph&& o)
	: charcode(o.charcode)
	, area(o.area)
	, horizontalBearing(o.horizontalBearing)
	, verticalBearing(o.verticalBearing)
	, advance(o.advance)
{
}

Font::Glyph::Glyph(int charcode, Rect4f area, Vector2f horizontalBearing, Vector2f verticalBearing, Vector2f advance)
	: charcode(charcode)
	, area(area)
	, horizontalBearing(horizontalBearing)
	, verticalBearing(verticalBearing)
	, advance(advance)
{
}

Font::Font(ResourceLoader& loader)
{
	auto root = YAML::Load(loader.getStatic()->getString());

	auto fontNode = root["font"];
	name = fontNode["name"].as<std::string>();
	height = fontNode["height"].as<float>();
	sizePt = fontNode["sizePt"].as<float>();

	auto texture = loader.getResource<Texture>("../font/" + fontNode["image"].as<std::string>());
	auto matDef = loader.getResource<MaterialDefinition>("distance_field_sprite.yaml");
	material = std::make_unique<Material>(matDef);
	(*material)["tex0"] = texture;

	auto glyphsNode = root["glyphs"];
	for (auto& node: glyphsNode) {
		int code = node["code"].as<int>();
		Rect4i iRect(node["x"].as<int>(), node["y"].as<int>(), node["w"].as<int>(), node["h"].as<int>());
		Rect4f rect = Rect4f(iRect) / Vector2f(texture->getSize());
		Vector2f hBearing = Vector2f(node["horizontalBearingX"].as<float>(), node["horizontalBearingY"].as<float>());
		Vector2f vBearing = Vector2f(node["verticalBearingX"].as<float>(), node["verticalBearingY"].as<float>());
		Vector2f advance = Vector2f(node["advanceX"].as<float>(), node["advanceY"].as<float>());

		glyphs.emplace(code, Glyph(code, rect, hBearing, vBearing, advance));
	}
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
