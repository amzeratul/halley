#include "network.h"

using namespace Halley;

Network::Network() {}

Network::~Network()
{
	stopListening();
	closeActiveConnections();
}

void Network::update()
{
	for (auto& c : active) {
		c->update();
	}
}

void Network::startListening(int port)
{
	listeningPort = port;
	// TODO
}

void Network::stopListening()
{
	listeningPort = -1;
	closePendingConnections();
	// TODO
}

std::shared_ptr<UDPConnection> Network::tryAcceptConnection()
{
	if (pending.size() > 0) {
		auto accept = pending.front();
		pending.erase(pending.begin());
		active.push_back(accept);
		accept->accept();
		return accept;
	}
	return std::shared_ptr<UDPConnection>();
}

std::shared_ptr<UDPConnection> Network::openConnection(String address, int port)
{
	auto result = std::make_shared<UDPConnection>(address, port);
	active.push_back(result);
	return result;
}

void Network::closePendingConnections()
{
	for (auto& c : pending) {
		c->close();
	}
	pending.clear();
}

void Network::closeActiveConnections()
{
	for (auto& c : active) {
		c->close();
	}
	active.clear();
}
