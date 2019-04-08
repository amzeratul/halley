#include "avf_movie_api.h"
#include <halley/plugin/plugin.h>

namespace Halley {

	class AVFPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new AVFMovieAPI(*system); }
		PluginType getType() override { return PluginType::MovieAPI; }
		String getName() override { return "Movie/AVFoundation"; }
		int getPriority() const override { return 1; }
	};

}

void initAVFPlugin(Halley::IPluginRegistry& registry)
{
	registry.registerPlugin(std::make_unique<Halley::AVFPlugin>());
}
