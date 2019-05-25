#include "android_system_api.h"
#include <halley/plugin/plugin.h>

namespace Halley {
	
	class AndroidPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new AndroidSystemAPI(); }
		PluginType getType() override { return PluginType::SystemAPI; }
		String getName() override { return "System/Android"; }
		int getPriority() const override { return 1; }
	};
	
}

void initAndroidPlugin(Halley::IPluginRegistry& registry)
{
	registry.registerPlugin(std::make_unique<Halley::AndroidPlugin>());
}
