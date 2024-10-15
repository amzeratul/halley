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

void AsioPlatformAPI::init() {}
void AsioPlatformAPI::deInit() {}

AsioPlatformAPI::AsioPlatformAPI(String playerName)
    : playerName(std::move(playerName))
{
}

String AsioPlatformAPI::getId()
{
    return "asio";
}

void AsioPlatformAPI::update() {}

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

std::shared_ptr<NetworkService> AsioPlatformAPI::createNetworkService(uint16_t port)
{
    return AsioNetworkAPI().createService(NetworkProtocol::TCP, port);
}
