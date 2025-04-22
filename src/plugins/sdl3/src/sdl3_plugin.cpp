#include "system_sdl3.h"
#include <halley/plugin/plugin.h>
#include "input_sdl3.h"
#include "audio_sdl3.h"

namespace Halley {
	
	class SDL3SystemPlugin : public Plugin {
	public:
		explicit SDL3SystemPlugin(std::optional<String> saveCryptKey)
			: saveCryptKey(std::move(saveCryptKey))
		{}

		HalleyAPIInternal* createAPI(SystemAPI*) override { return new SystemSDL3(saveCryptKey); }
		PluginType getType() override { return PluginType::SystemAPI; }
		String getName() override { return "System/SDL3"; }

	private:
		std::optional<String> saveCryptKey;
	};

	class SDL3InputPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new InputSDL3(*system); }
		PluginType getType() override { return PluginType::InputAPI; }
		String getName() override { return "Input/SDL3"; }
	};

	class SDL3AudioPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new AudioSDL3(); }
		PluginType getType() override { return PluginType::AudioOutputAPI; }
		String getName() override { return "Audio/SDL3"; }
	};
	
}

void initSDL3SystemPlugin(Halley::IPluginRegistry &registry, std::optional<Halley::String> saveCryptKey)
{
	registry.registerPlugin(std::make_unique<Halley::SDL3SystemPlugin>(std::move(saveCryptKey)));
}

void initSDL3InputPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::SDL3InputPlugin>());
}

void initSDL3AudioPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::SDL3AudioPlugin>());
}
