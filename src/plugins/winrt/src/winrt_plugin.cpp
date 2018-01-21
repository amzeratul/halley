#include "winrt_system.h"
#include <halley/plugin/plugin.h>

namespace Halley {
	
	class WinRTPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new WinRTSystem(); }
		PluginType getType() override { return PluginType::SystemAPI; }
		String getName() override { return "System/WinRT"; }
	};
	
}

void initWinRTPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::WinRTPlugin>());
}
