#include "asio_network_api.h"
#include "asio_network_service.h"

using namespace Halley;

std::unique_ptr<NetworkService> AsioNetworkAPI::createService(int port)
{
	return std::make_unique<AsioNetworkService>(port);
}

void AsioNetworkAPI::init() {}
void AsioNetworkAPI::deInit() {}
