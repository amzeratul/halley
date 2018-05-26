#include "asio_tcp_network_service.h"
using namespace Halley;

AsioTCPNetworkService::AsioTCPNetworkService(int port)
{
}

void AsioTCPNetworkService::update()
{
}

void AsioTCPNetworkService::setAcceptingConnections(bool accepting)
{
}

std::shared_ptr<IConnection> AsioTCPNetworkService::tryAcceptConnection()
{
	return {};
}

std::shared_ptr<IConnection> AsioTCPNetworkService::connect(String address, int port)
{
	return {};
}
