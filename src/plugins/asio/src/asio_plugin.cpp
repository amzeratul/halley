#include <halley/plugin/plugin.h>
#include "asio_network_api.h"

namespace Halley {
	
	class AsioPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new AsioNetworkAPI(); }
		PluginType getType() override { return PluginType::NetworkAPI; }
		String getName() override { return "Network/ASIO"; }
	};

    class AsioPlatformPlugin : public Plugin {
    public:
        explicit AsioPlatformPlugin(String playerName) : playerName(std::move(playerName)) {}
    private:
        HalleyAPIInternal* createAPI(SystemAPI*) override { return new AsioPlatformAPI(playerName); }
        PluginType getType() override { return PluginType::PlatformAPI; }
        String getName() override { return "Platform/ASIO"; }
        String playerName;
    };

}

void initAsioPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::AsioPlugin>());
}

void initAsioPlatformPlugin(Halley::IPluginRegistry& registry, const Halley::String& playerName)
{
    registry.registerPlugin(std::make_unique<Halley::AsioPlatformPlugin>(playerName));
}