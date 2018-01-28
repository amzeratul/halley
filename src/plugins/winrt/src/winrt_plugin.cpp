#include "winrt_system.h"
#include <halley/plugin/plugin.h>
#include "winrt_input.h"
#include "winrt_audio_output.h"

namespace Halley {
	
	class WinRTSystemPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new WinRTSystem(); }
		PluginType getType() override { return PluginType::SystemAPI; }
		String getName() override { return "System/WinRT"; }
	};

	class WinRTInputPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new WinRTInput(); }
		PluginType getType() override { return PluginType::InputAPI; }
		String getName() override { return "Input/WinRT"; }
	};

	class WinRTAudioPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new WinRTAudioOutput(); }
		PluginType getType() override { return PluginType::AudioOutputAPI; }
		String getName() override { return "AudioOutput/WinRT"; }
	};
	
}

void initWinRTPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::WinRTSystemPlugin>());
	registry.registerPlugin(std::make_unique<Halley::WinRTInputPlugin>());
	registry.registerPlugin(std::make_unique<Halley::WinRTAudioPlugin>());
}
