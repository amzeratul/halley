#pragma once

#include <unordered_map>

#include "halley/maths/colour.h"

namespace Halley {
	class ConfigNode;

	class UIColourScheme {
    public:
		UIColourScheme();
        UIColourScheme(const ConfigNode& config);
    	
        Colour4f getColour(const String& key) const;

    private:
		Colour4f defaultColour;
        std::unordered_map<String, Colour4f> colours;
    };
}
