#include "system_sdl.h"
#include <halley/plugin/plugin.h>
#include "input_sdl.h"
#include "audio_sdl.h"

namespace Halley {
	
	class SDLSystemPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new SystemSDL(); }
		PluginType getType() override { return PluginType::SystemAPI; }
		String getName() override { return "System/SDL"; }
	};

	class SDLInputPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new InputSDL(); }
		PluginType getType() override { return PluginType::InputAPI; }
		String getName() override { return "Input/SDL"; }
	};

	class SDLAudioPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new AudioSDL(); }
		PluginType getType() override { return PluginType::AudioOutputAPI; }
		String getName() override { return "Audio/SDL"; }
	};
	
}

void initSDLSystemPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::SDLSystemPlugin>());
}

void initSDLInputPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::SDLInputPlugin>());
}

void initSDLAudioPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::SDLAudioPlugin>());
}
