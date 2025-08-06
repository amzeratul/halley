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
        explicit AsioPlatformPlugin(String playerName, const std::optional<String>& joinLobbyAddress) : playerName(std::move(playerName)), joinLobbyAddress(joinLobbyAddress) {}
    private:
        HalleyAPIInternal* createAPI(SystemAPI*) override { return new AsioPlatformAPI(playerName, joinLobbyAddress); }
        PluginType getType() override { return PluginType::PlatformAPI; }
        String getName() override { return "Platform/ASIO"; }
        String playerName;
    	std::optional<String> joinLobbyAddress;
    };

}

void initAsioPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::AsioPlugin>());
}

void initAsioPlatformPlugin(Halley::IPluginRegistry& registry, const Halley::String& playerName, const std::optional<Halley::String>& joinLobbyAddress)
{
    registry.registerPlugin(std::make_unique<Halley::AsioPlatformPlugin>(playerName, joinLobbyAddress));
}