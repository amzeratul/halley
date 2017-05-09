#include "session/network_session.h"
#include "connection/network_service.h"
#include "connection/network_packet.h"
using namespace Halley;

NetworkSession::NetworkSession(NetworkService& service)
	: service(service)
{
}

NetworkSession::~NetworkSession()
{
	if (type == NetworkSessionType::Host) {
		service.setAcceptingConnections(false);
	}
	NetworkSession::close();
}

void NetworkSession::host(int port)
{
	Expects(type == NetworkSessionType::Undefined);
	type = NetworkSessionType::Host;
}

void NetworkSession::join(const String& address, int port)
{
	Expects(type == NetworkSessionType::Undefined);
	type = NetworkSessionType::Client;
	connections.emplace_back(service.connect(address, port));
}

void NetworkSession::setMaxClients(int clients)
{
	maxClients = clients;
}

int NetworkSession::getClientCount() const
{
	if (type == NetworkSessionType::Client) {
		return getStatus() != ConnectionStatus::OPEN ? 0 : 2; // TODO
	} else if (type == NetworkSessionType::Host) {
		int i = 1;
		for (auto& c: connections) {
			if (c->getStatus() == ConnectionStatus::OPEN) {
				++i;
			}
		}
		return i;
	} else {
		return 0;
	}
}

void NetworkSession::update()
{
	// Remove dead connections
	service.update();
	connections.erase(std::remove_if(connections.begin(), connections.end(), [] (const std::shared_ptr<IConnection>& c) { return c->getStatus() == ConnectionStatus::CLOSED; }), connections.end());

	if (type == NetworkSessionType::Host) {
		if (getClientCount() < maxClients) { // I'm also a client!
			service.setAcceptingConnections(true);
			auto incoming = service.tryAcceptConnection();
			if (incoming) {
				connections.emplace_back(std::move(incoming));
			}
		} else {
			service.setAcceptingConnections(false);
		}
	}

	if (type == NetworkSessionType::Client && connections.empty()) {
		close();
	}
}

NetworkSessionType NetworkSession::getType() const
{
	return type;
}

bool NetworkSession::hasSharedData(const String& id) const
{
	return sharedData.find(id) != sharedData.end();
}

DeserializeHelper NetworkSession::getSharedData(const String& id) const
{
	auto iter = sharedData.find(id);
	if (iter == sharedData.end()) {
		throw Exception("Shared data not found: " + id);
	}
	return DeserializeHelper(iter->second);
}

void NetworkSession::setSharedData(const String& id, Bytes&& data)
{
	sharedData[id] = std::move(data);
}

void NetworkSession::close()
{
	for (auto& c: connections) {
		c->close();
	}
	connections.clear();
	type = NetworkSessionType::Undefined;
}

ConnectionStatus NetworkSession::getStatus() const
{
	if (type == NetworkSessionType::Undefined) {
		return ConnectionStatus::UNDEFINED;
	} else if (type == NetworkSessionType::Client) {
		if (connections.empty()) {
			return ConnectionStatus::CLOSED;
		} else {
			return connections[0]->getStatus();
		}
	} else if (type == NetworkSessionType::Host) {
		return ConnectionStatus::OPEN;
	} else {
		throw Exception("Unknown session type.");
	}
}

void NetworkSession::send(OutboundNetworkPacket&& packet)
{
	for (auto& c: connections) {
		c->send(std::move(packet));
	}
}

bool NetworkSession::receive(InboundNetworkPacket& packet)
{
	if (connections.empty()) {
		return false;
	} else {
		for (size_t i = 0; i < connections.size(); ++i) {
			bool gotMessage = connections[i]->receive(packet);
			if (gotMessage) {
				// Broadcast to other connections
				for (size_t j = 0; j < connections.size(); ++j) {
					if (i != j) {
						connections[j]->send(OutboundNetworkPacket(packet.getBytes()));
					}
				}
				return true;
			}
		}
		return false;
	}	
}
