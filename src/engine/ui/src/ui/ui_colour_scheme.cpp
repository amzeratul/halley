#include "halley/ui/ui_colour_scheme.h"

#include "halley/core/resources/resources.h"
#include "halley/data_structures/config_node.h"
#include "halley/file_formats/config_file.h"
#include "halley/support/logger.h"

using namespace Halley;

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
	if (node.hasKey("defaultColour")) {
		defaultColour = Colour4f::fromString(node["defaultColour"].asString());
	}
	name = node["name"].asString();
}

Colour4f UIColourScheme::getColour(const String& key) const
{
	const auto iter = colours.find(key);
	if (iter != colours.end()) {
		return iter->second;
	}

	Logger::logWarning("Colour scheme does not define key \"" + key + "\".");
	return defaultColour;
}

const String& UIColourScheme::getName() const
{
	return name;
}
