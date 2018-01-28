#include "winrt_system.h"
#include <halley/plugin/plugin.h>
#include "winrt_input.h"
#include "xaudio2_audio_output.h"
#include "winrt_platform.h"

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
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new XAudio2AudioOutput(); }
		PluginType getType() override { return PluginType::AudioOutputAPI; }
		String getName() override { return "AudioOutput/XAudio2"; }
	};

	class WinRTPlatformPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new WinRTPlatform(); }
		PluginType getType() override { return PluginType::PlatformAPI; }
		String getName() override { return "Platform/WinRT"; }
	};
	
}

void initWinRTPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::WinRTSystemPlugin>());
	registry.registerPlugin(std::make_unique<Halley::WinRTInputPlugin>());
	registry.registerPlugin(std::make_unique<Halley::WinRTAudioPlugin>());
	registry.registerPlugin(std::make_unique<Halley::WinRTPlatformPlugin>());
}
