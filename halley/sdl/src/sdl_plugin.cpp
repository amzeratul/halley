#include "system_sdl.h"
#include <halley/plugin/plugin.h>

namespace Halley {
	
	class SDLPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new SystemSDL(); }
		PluginType getType() override { return PluginType::SystemAPI; }
		String getName() override { return "System/SDL"; }
	};
	
}

void initSDLPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::SDLPlugin>());
}
