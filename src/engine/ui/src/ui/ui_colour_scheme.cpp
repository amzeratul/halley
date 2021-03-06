#include "halley/ui/ui_colour_scheme.h"


#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/resources/resources.h"
#include "halley/data_structures/config_node.h"
#include "halley/file_formats/config_file.h"
#include "halley/support/logger.h"

using namespace Halley;

static Sprite getSpriteFromConfigNode(const ConfigNode& node, Resources& resources)
{
	Sprite sprite;

	if (node.getType() == ConfigNodeType::String) {
		sprite = Sprite().setImage(resources, node.asString());
	} else {
		sprite = Sprite()
			.setImage(resources, node["img"].asString(), node["material"].asString(""))
			.setCustom0(node["custom0"].asVector4f(Vector4f(0, 0, 0, 0)));
	}

	return sprite;
}

UIColourScheme::UIColourScheme()
{}

UIColourScheme::UIColourScheme(const ConfigNode& node, Resources& resources)
{
	if (node.hasKey("base")) {
		const auto& base = resources.get<ConfigFile>(node["base"].asString());
		*this = UIColourScheme(base->getRoot(), resources);
	}
	
	for (const auto& [k, v]: node["colours"].asMap()) {
		colours[k] = Colour4f::fromString(v.asString());
	}

	if (node.hasKey("sprites")) {
		for (const auto& [k, v]: node["sprites"].asMap()) {
			sprites[k] = getSpriteFromConfigNode(v, resources);
		}
	}
	
	if (node.hasKey("defaultColour")) {
		defaultColour = Colour4f::fromString(node["defaultColour"].asString());
	}
	name = node["name"].asString();

	enabled = node["enabled"].asBool(true);

	{
		auto mat = std::make_shared<Material>(resources.get<MaterialDefinition>("Halley/Scanlines"));
		mat
			->set("u_col0", getColour("background0"))
			.set("u_col1", getColour("background1"))
			.set("u_distance", 6.0f);
		background = Sprite().setMaterial(mat);
	}
	if (node.hasKey("backgroundParticles")) {
		backgroundParticles = Particles(node["backgroundParticles"]["particles"], resources);
		std::vector<Sprite> particleSprites;
		for (const auto& spriteNode: node["backgroundParticles"]["sprites"].asSequence()) {
			particleSprites.push_back(getSpriteFromConfigNode(spriteNode, resources));
		}
		backgroundParticles.setSprites(std::move(particleSprites));
	}
}

Colour4f UIColourScheme::getColour(const String& rawKey) const
{
	if (rawKey.isEmpty()) {
		Logger::logWarning("Empty key requested from Colour scheme");
		return Colour4f(1, 1, 1, 1);
	}
	
	if (rawKey[0] == '#') {
		return Colour4f::fromString(rawKey);
	}

	const String& key = rawKey[0] == '$' ? rawKey.mid(1) : rawKey;
	
	const auto iter = colours.find(key);
	if (iter != colours.end()) {
		return iter->second;
	}

	Logger::logWarning("Colour scheme does not define key \"" + key + "\".");
	return defaultColour;
}

Sprite UIColourScheme::getSprite(Resources& resources, const String& name, const String& material) const
{
	if (name.startsWith("$")) {
		const auto iter = sprites.find(name.mid(1));
		if (iter != sprites.end()) {
			return iter->second;
		}

		Logger::logWarning("Colour scheme does not define sprite \"" + name.mid(1) + "\".");
		return Sprite();
	} else {
		return Sprite().setImage(resources, name, material);
	}
}

const Sprite& UIColourScheme::getBackground() const
{
	return background;
}

const Particles& UIColourScheme::getBackgroundParticles() const
{
	return backgroundParticles;
}

const String& UIColourScheme::getName() const
{
	return name;
}

bool UIColourScheme::isEnabled() const
{
	return enabled;
}
