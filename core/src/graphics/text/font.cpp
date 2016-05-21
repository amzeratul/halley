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
	, size(o.size)
	, horizontalBearing(o.horizontalBearing)
	, verticalBearing(o.verticalBearing)
	, advance(o.advance)
{
}

Font::Glyph::Glyph(int charcode, Rect4f area, Vector2f size, Vector2f horizontalBearing, Vector2f verticalBearing, Vector2f advance)
	: charcode(charcode)
	, area(area)
	, size(size)
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
	ascender = fontNode["ascender"].as<float>();
	height = fontNode["height"].as<float>();
	sizePt = fontNode["sizePt"].as<float>();
	smoothRadius = fontNode["radius"].as<float>();

	auto texture = loader.getAPI().getResource<Texture>("../font/" + fontNode["image"].as<std::string>());
	auto matDef = loader.getAPI().getResource<MaterialDefinition>("text.yaml");
	material = std::make_unique<Material>(matDef);
	(*material)["tex0"] = texture;

	auto glyphsNode = root["glyphs"];
	for (auto& node: glyphsNode) {
		int code = node["code"].as<int>();
		auto bearingNode = node["bearing"];
		auto advanceNode = node["advance"];
		auto rectNode = node["rect"];

		Rect4i iRect(rectNode[0].as<int>(), rectNode[1].as<int>(), rectNode[2].as<int>(), rectNode[3].as<int>());
		Rect4f rect = Rect4f(iRect) / Vector2f(texture->getSize());
		Vector2f hBearing = Vector2f(bearingNode[0].as<float>(), bearingNode[1].as<float>());
		Vector2f vBearing = Vector2f(bearingNode[2].as<float>(), bearingNode[3].as<float>());
		Vector2f advance = Vector2f(advanceNode[0].as<float>(), advanceNode[1].as<float>());

		glyphs.emplace(code, Glyph(code, rect, Vector2f(iRect.getSize()), hBearing, vBearing, advance));
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
