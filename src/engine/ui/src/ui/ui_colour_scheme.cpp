#include "halley/ui/ui_colour_scheme.h"
#include "halley/data_structures/config_node.h"
#include "halley/support/logger.h"

using namespace Halley;

UIColourScheme::UIColourScheme()
{}

UIColourScheme::UIColourScheme(const ConfigNode& node)
{
	for (const auto& [k, v]: node["colours"].asMap()) {
		colours[k] = Colour4f::fromString(v.asString());
	}
	defaultColour = Colour4f::fromString(node["defaultColour"].asString());
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
