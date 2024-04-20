#include "system_sdl.h"
#include <halley/plugin/plugin.h>
#include "input_sdl.h"
#include "audio_sdl.h"

namespace Halley {
	
	class SDLSystemPlugin : public Plugin {
	public:
		SDLSystemPlugin(std::optional<String> saveCryptKey)
			: saveCryptKey(std::move(saveCryptKey))
		{}

		HalleyAPIInternal* createAPI(SystemAPI*) override { return new SystemSDL(saveCryptKey); }
		PluginType getType() override { return PluginType::SystemAPI; }
		String getName() override { return "System/SDL"; }

	private:
		std::optional<String> saveCryptKey;
	};

	class SDLInputPlugin : public Plugin {
	public:
		SDLInputPlugin(bool allowXInput) : allowXInput(allowXInput) {}
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new InputSDL(*system, allowXInput); }
		PluginType getType() override { return PluginType::InputAPI; }
		String getName() override { return "Input/SDL"; }

		bool allowXInput;
	};

	class SDLAudioPlugin : public Plugin {
	public:
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new AudioSDL(); }
		PluginType getType() override { return PluginType::AudioOutputAPI; }
		String getName() override { return "Audio/SDL"; }
	};
	
}

void initSDLSystemPlugin(Halley::IPluginRegistry &registry, std::optional<Halley::String> saveCryptKey)
{
	registry.registerPlugin(std::make_unique<Halley::SDLSystemPlugin>(std::move(saveCryptKey)));
}

void initSDLInputPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::SDLInputPlugin>(true));
}

void initSDLInputPlugin(Halley::IPluginRegistry &registry, bool allowXInput)
{
	registry.registerPlugin(std::make_unique<Halley::SDLInputPlugin>(allowXInput));
}

void initSDLAudioPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::SDLAudioPlugin>());
}
