#include <halley/plugin/plugin.h>
#include "socketio_network_api.h"

namespace Halley {
	
	class SocketIOPlugin : public Plugin {
		HalleyAPIInternal* createAPI(SystemAPI*) override { return new SocketIONetworkAPI(); }
		PluginType getType() override { return PluginType::NetworkAPI; }
		String getName() override { return "Network/SocketIO"; }
	};

    class SocketIOPlatformPlugin : public Plugin {
    public:
        explicit SocketIOPlatformPlugin(String playerName, const std::optional<String>& joinLobbyAddress) : playerName(std::move(playerName)), joinLobbyAddress(joinLobbyAddress) {}
    private:
        HalleyAPIInternal* createAPI(SystemAPI*) override { return new SocketIOPlatformAPI(playerName, joinLobbyAddress); }
        PluginType getType() override { return PluginType::PlatformAPI; }
        String getName() override { return "Platform/SocketIO"; }
        String playerName;
    	std::optional<String> joinLobbyAddress;
    };

}

void initSocketIOPlugin(Halley::IPluginRegistry &registry)
{
	registry.registerPlugin(std::make_unique<Halley::SocketIOPlugin>());
}

void initSocketIOPlatformPlugin(Halley::IPluginRegistry& registry, const Halley::String& playerName, const std::optional<Halley::String>& joinLobbyAddress)
{
    registry.registerPlugin(std::make_unique<Halley::SocketIOPlatformPlugin>(playerName, joinLobbyAddress));
}