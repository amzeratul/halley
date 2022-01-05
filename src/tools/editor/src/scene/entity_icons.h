#pragma once

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
        const Sprite& getInvalidEntityIcon(IEntityValidator::Severity severity) const;

    	const std::vector<Entry>& getEntries() const;

    private:
    	Resources& resources;
    	const UIColourScheme& colourScheme;
    	
    	std::vector<Entry> entries;
    	HashMap<String, size_t> entryMap;
    	Entry defaultEntry;
        Sprite invalidEntityWarningIcon;
    	Sprite invalidEntityErrorIcon;

    	const Entry& getEntry(const String& id) const;
       	void load(const ConfigNode& node);
	};
}
