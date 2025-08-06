#include "asio_network_api.h"
#include "asio_tcp_network_service.h"
#include "asio_udp_network_service.h"

using namespace Halley;

std::unique_ptr<NetworkService> AsioNetworkAPI::createService(NetworkProtocol protocol, int port)
{
	if (protocol == NetworkProtocol::TCP) {
		return std::make_unique<AsioTCPNetworkService>(port);
	} else if (protocol == NetworkProtocol::UDP) {
		return std::make_unique<AsioUDPNetworkService>(port);
	} else {
		return {};
	}
}

void AsioNetworkAPI::init() {}
void AsioNetworkAPI::deInit() {}

AsioPlatformAPI::AsioPlatformAPI(String playerName, const std::optional<String>& joinLobbyAddress)
    : playerName(std::move(playerName))
    , joinLobbyAddress(joinLobbyAddress)
{
}

void AsioPlatformAPI::init() {}
void AsioPlatformAPI::deInit() {}

String AsioPlatformAPI::getId()
{
    return "asio";
}

void AsioPlatformAPI::update()
{
    if (preparingInvitation && preparingToJoinCallback) {
        preparingToJoinCallback();
        preparingInvitation = false;
        readyInvitation = true;
    }

    if (readyInvitation && joinCallback) {
        joinCallback(PlatformJoinCallbackParameters{joinLobbyAddress.value_or("127.0.0.1:6060")});
        readyInvitation = false;
    }
}

String AsioPlatformAPI::getPlayerName()
{
    return playerName;
}

String AsioPlatformAPI::getAccountId()
{
    return playerName;
}

bool AsioPlatformAPI::canProvideAuthToken() const
{
    return true;
}

Future<AuthTokenResult> AsioPlatformAPI::getAuthToken(const Halley::AuthTokenParameters& parameters)
{
    OnlineCapabilities capabilities;
    capabilities.onlinePlay = true;
    AuthTokenResult result(AuthTokenRetrievalResult::OK, capabilities);

    Promise<AuthTokenResult> promise;
    promise.setValue(std::move(result));

    return promise.getFuture();
}

void AsioPlatformAPI::showBrowseGamesToJoinUI()
{
    preparingInvitation = true;
}

void AsioPlatformAPI::setJoinCallback(PlatformJoinCallback callback)
{
    joinCallback = callback;
}

void AsioPlatformAPI::setPreparingToJoinCallback(PlatformPreparingToJoinCallback callback)
{
    preparingToJoinCallback = callback;
}

std::shared_ptr<NetworkService> AsioPlatformAPI::createNetworkService(uint16_t port)
{
    return AsioNetworkAPI().createService(NetworkProtocol::UDP, port);
}
