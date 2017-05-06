#include <halley/plugin/plugin.h>
#include "asio_network_api.h"

namespace Halley {
	
	class AsioPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI* system) override { return new AsioNetworkAPI(); }
		PluginType getType() override { return PluginType::NetworkAPI; }
		String getName() override { return "Network/ASIO"; }
	};
	
}

void initAsioPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::AsioPlugin>());
}
