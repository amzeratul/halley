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
