#include "entity_icons.h"
using namespace Halley;

EntityIcons::EntityIcons(Resources& resources, const UIColourScheme& colourScheme)
	: resources(resources)
	, colourScheme(colourScheme)
{
	for (const auto& configId: resources.enumerate<ConfigFile>()) {
		if (configId.startsWith("entity_icons")) {
			load(resources.get<ConfigFile>(configId)->getRoot());
		}
	}
}

const EntityIcons::Entry& EntityIcons::getEntry(const String& id) const
{
	const auto iter = entries.find(id);
	if (iter != entries.end()) {
		return iter->second;
	}
	return defaultEntry;
}

void EntityIcons::load(const ConfigNode& node)
{
	const auto& seq = node["entityIcons"].asSequence();
	entries.reserve(entries.size() + seq.size());
	
	for (const auto& n: seq) {
		auto entry = Entry(resources, n, colourScheme);
		String id = entry.id;
		
		if (id.isEmpty()) {
			defaultEntry = entry;
		}
		entries[id] = std::move(entry);
		ids.push_back(std::move(id));
	}
}

const Sprite& EntityIcons::getIcon(const String& id) const
{
	return getEntry(id).icon;
}

const String& EntityIcons::getName(const String& id) const
{
	return getEntry(id).name;
}

const std::vector<String>& EntityIcons::getIconIds() const
{
	return ids;
}

EntityIcons::Entry::Entry(Resources& resources, const ConfigNode& node, const UIColourScheme& colourScheme)
{
	id = node["id"].asString();
	name = node["name"].asString();
	icon = Sprite()
		.setImage(resources, node["image"].asString())
		.setColour(colourScheme.getColour(node["colour"].asString()));
}
