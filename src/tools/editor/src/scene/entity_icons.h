#pragma once

#include <unordered_map>

namespace Halley {
    class EntityIcons {
    public:
    	EntityIcons(Resources& resources, const UIColourScheme& colourScheme);

    	const Sprite& getIcon(const String& id) const;
    	const String& getName(const String& id) const;
    	const std::vector<String>& getIconIds() const;

    private:
    	Resources& resources;
    	const UIColourScheme& colourScheme;
    	
    	class Entry {
    	public:
    		Entry() = default;
    		Entry(Resources& resources, const ConfigNode& node, const UIColourScheme& colourScheme);

    		Sprite icon;
    		String name;
    		String id;
    	};

    	std::vector<String> ids;
    	std::unordered_map<String, Entry> entries;
    	Entry defaultEntry;

    	const Entry& getEntry(const String& id) const;
       	void load(const ConfigNode& node);
	};
}
