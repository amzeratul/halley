#pragma once

#include <unordered_map>

namespace Halley {
    class EntityIcons {
    public:
    	class Entry {
    	public:
    		Entry() = default;
    		Entry(Resources& resources, const ConfigNode& node, const UIColourScheme& colourScheme);

    		Sprite icon;
    		String name;
    		String id;
    	};

    	EntityIcons(Resources& resources, const UIColourScheme& colourScheme);

    	const Sprite& getIcon(const String& id) const;
    	const String& getName(const String& id) const;

    	const std::vector<Entry>& getEntries() const;

    private:
    	Resources& resources;
    	const UIColourScheme& colourScheme;
    	
    	std::vector<Entry> entries;
    	std::unordered_map<String, size_t> entryMap;
    	Entry defaultEntry;

    	const Entry& getEntry(const String& id) const;
       	void load(const ConfigNode& node);
	};
}
