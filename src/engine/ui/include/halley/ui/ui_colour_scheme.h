#pragma once

#include "halley/core/graphics/sprite/particles.h"
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
		bool isEnabled() const;

		Colour4f getColour(const String& key) const;
		Sprite getSprite(Resources& resources, const String& name, const String& material) const;

		const Sprite& getBackground() const;
		const Particles& getBackgroundParticles() const;

	private:
		String name;
		Colour4f defaultColour;
		bool enabled = true;
        HashMap<String, Colour4f> colours;
		HashMap<String, Sprite> sprites;

		Sprite background;
		Particles backgroundParticles;
    };
}
