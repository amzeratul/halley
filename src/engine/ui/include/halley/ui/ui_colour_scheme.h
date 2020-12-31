#pragma once

#include <unordered_map>

#include "halley/maths/colour.h"

namespace Halley {
	class Resources;
	class ConfigNode;

	class UIColourScheme {
    public:
		UIColourScheme();
        UIColourScheme(const ConfigNode& config, Resources& resources);
    	
        Colour4f getColour(const String& key) const;
		const String& getName() const;

	private:
		String name;
		Colour4f defaultColour;
        std::unordered_map<String, Colour4f> colours;
    };
}
