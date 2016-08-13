#include "network_service.h"

using namespace Halley;

NetworkService::NetworkService() {}

NetworkService::~NetworkService()
{
}

void NetworkService::update()
{
}

void NetworkService::startListening(int port)
{
}

void NetworkService::stopListening()
{
}

std::shared_ptr<UDPConnection> NetworkService::tryAcceptConnection()
{
	return std::shared_ptr<UDPConnection>();
}

std::shared_ptr<UDPConnection> NetworkService::connect(String address, int port)
{
	return std::shared_ptr<UDPConnection>();
}

void NetworkService::closePendingConnections()
{

}

void NetworkService::closeActiveConnections()
{
}
