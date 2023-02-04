#include <halley/plugin/plugin.h>
#include "xaudio2_audio_output.h"

namespace Halley {
	
	class XAudio2Plugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new XAudio2AudioOutput(); }
		PluginType getType() override { return PluginType::AudioOutputAPI; }
		String getName() override { return "AudioOutput/XAudio2"; }
	};
	
}

void initXAudio2Plugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::XAudio2Plugin>());
}
