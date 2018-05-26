#include "dummy_network.h"

using namespace Halley;

void DummyNetworkAPI::init() {}
void DummyNetworkAPI::deInit() {}

std::unique_ptr<NetworkService> DummyNetworkAPI::createService(NetworkProtocol protocol, int port)
{
	return std::make_unique<DummyNetworkService>();
}

void DummyNetworkService::update()
{	
}

void DummyNetworkService::setAcceptingConnections(bool accepting)
{
}

std::shared_ptr<IConnection> DummyNetworkService::tryAcceptConnection()
{
	return std::shared_ptr<IConnection>();
}

std::shared_ptr<IConnection> DummyNetworkService::connect(String address, int port)
{
	return std::shared_ptr<IConnection>();
}
