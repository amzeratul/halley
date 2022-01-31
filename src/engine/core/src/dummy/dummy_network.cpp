#include "dummy_network.h"

using namespace Halley;

void DummyNetworkAPI::init() {}
void DummyNetworkAPI::deInit() {}

std::unique_ptr<NetworkService> DummyNetworkAPI::createService(NetworkProtocol protocol, int port)
{
	return std::make_unique<DummyNetworkService>();
}

void DummyNetworkService::update(Time t)
{}

String DummyNetworkService::startListening(AcceptCallback callback)
{
	return "";
}

void DummyNetworkService::stopListening()
{}

std::shared_ptr<IConnection> DummyNetworkService::connect(const String& address)
{
	return std::shared_ptr<IConnection>();
}
