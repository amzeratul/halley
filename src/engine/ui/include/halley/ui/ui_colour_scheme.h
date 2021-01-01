#pragma once

#include <unordered_map>


#include "halley/core/graphics/sprite/sprite.h"
#include "halley/maths/colour.h"

namespace Halley {
	class Resources;
	class ConfigNode;

	class UIColourScheme {
    public:
		UIColourScheme();
        UIColourScheme(const ConfigNode& config, Resources& resources);
    	
		const String& getName() const;

		Colour4f getColour(const String& key) const;
		Sprite getSprite(Resources& resources, const String& name, const String& material) const;

	private:
		String name;
		Colour4f defaultColour;
        std::unordered_map<String, Colour4f> colours;
		std::unordered_map<String, Sprite> sprites;
    };
}
