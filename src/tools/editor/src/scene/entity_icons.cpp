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

	invalidEntityErrorIcon.setImage(resources, "entity_icons/warning.png").setColour(colourScheme.getColour("taskError"));
	invalidEntityWarningIcon.setImage(resources, "entity_icons/warning.png").setColour(colourScheme.getColour("taskWarning"));
}

const EntityIcons::Entry& EntityIcons::getEntry(const String& id) const
{
	const auto iter = entryMap.find(id);
	if (iter != entryMap.end()) {
		return entries[iter->second];
	}
	return defaultEntry;
}

void EntityIcons::load(const ConfigNode& node)
{
	const auto& seq = node["entityIcons"].asSequence();
	entries.reserve(entries.size() + seq.size());
	entryMap.reserve(entryMap.size() + seq.size());
	
	for (const auto& n: seq) {
		auto entry = Entry(resources, n, colourScheme);
		const String id = entry.id;
		
		if (id.isEmpty()) {
			defaultEntry = entry;
		}
		entryMap[id] = entries.size();
		entries.emplace_back(std::move(entry));
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

const Sprite& EntityIcons::getInvalidEntityIcon(IEntityValidator::Severity severity) const
{
	return severity == IEntityValidator::Severity::Error ? invalidEntityErrorIcon : invalidEntityWarningIcon;
}

const Vector<EntityIcons::Entry>& EntityIcons::getEntries() const
{
	return entries;
}

EntityIcons::Entry::Entry(Resources& resources, const ConfigNode& node, const UIColourScheme& colourScheme)
{
	id = node["id"].asString();
	name = node["name"].asString();
	icon = Sprite()
		.setImage(resources, node["image"].asString())
		.setColour(colourScheme.getColour(node["colour"].asString()));
}
