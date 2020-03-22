#include "win32_system.h"
#include <halley/plugin/plugin.h>

namespace Halley {
	
	class Win32SystemPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new Win32System(); }
		PluginType getType() override { return PluginType::SystemAPI; }
		String getName() override { return "System/Win32"; }
	};

}

void initWin32Plugin(Halley::IPluginRegistry &registry, Halley::Maybe<Halley::String> cryptKey)
{
	registry.registerPlugin(std::make_unique<Halley::Win32SystemPlugin>());
}
