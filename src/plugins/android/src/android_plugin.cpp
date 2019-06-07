#include "system/android_system_api.h"
#include "input/android_input_api.h"
#include <halley/plugin/plugin.h>

namespace Halley {
	
	class AndroidSystemPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new AndroidSystemAPI(); }
		PluginType getType() override { return PluginType::SystemAPI; }
		String getName() override { return "System/Android"; }
		int getPriority() const override { return 1; }
	};

	class AndroidInputPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new AndroidInputAPI(*static_cast<AndroidSystemAPI*>(system)); }
		PluginType getType() override { return PluginType::InputAPI; }
		String getName() override { return "Input/Android"; }
		int getPriority() const override { return 1; }
	};
	
}

void initAndroidPlugin(Halley::IPluginRegistry& registry)
{
	registry.registerPlugin(std::make_unique<Halley::AndroidSystemPlugin>());
	registry.registerPlugin(std::make_unique<Halley::AndroidInputPlugin>());
}
