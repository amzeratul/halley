#include "mf_movie_api.h"
#include <halley/plugin/plugin.h>

namespace Halley {
	
	class MFPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new MFMovieAPI(); }
		PluginType getType() override { return PluginType::MovieAPI; }
		String getName() override { return "Movie/MediaFoundation"; }
		int getPriority() const override { return 1; }
	};
	
}

void initMFPlugin(Halley::IPluginRegistry& registry)
{
	registry.registerPlugin(std::make_unique<Halley::MFPlugin>());
}
