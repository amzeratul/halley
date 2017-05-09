#include "session/network_session.h"
#include "session/network_session_control_messages.h"
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
	playerId = 0;
}

void NetworkSession::join(const String& address, int port)
{
	Expects(type == NetworkSessionType::Undefined);

	connections.emplace_back(service.connect(address, port));
	
	type = NetworkSessionType::Client;
	playerId = 1; // TODO: get from master
}

void NetworkSession::close()
{
	for (auto& c: connections) {
		c->close();
	}
	connections.clear();

	type = NetworkSessionType::Undefined;
	playerId = -1;
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

OutboundNetworkPacket NetworkSession::makeOutbound(gsl::span<const gsl::byte> data, NetworkSessionMessageHeader header)
{
	auto packet = OutboundNetworkPacket(data);
	packet.addHeader(header);
	return packet;
}

void NetworkSession::send(OutboundNetworkPacket&& packet)
{
	NetworkSessionMessageHeader header;
	header.type = NetworkSessionMessageType::ToPeers;
	header.srcPeerId = playerId;

	auto out = makeOutbound(packet.getBytes(), header);
	for (auto& c: connections) {
		c->send(OutboundNetworkPacket(out));
	}
}

bool NetworkSession::receive(InboundNetworkPacket& packet)
{
	for (size_t i = 0; i < connections.size(); ++i) {
		bool gotMessage = connections[i]->receive(packet);
		if (gotMessage) {
			// Get header
			int peerId = type == NetworkSessionType::Host ? int(i) + 1 : 0;
			NetworkSessionMessageHeader header;
			packet.extractHeader(header);

			if (type == NetworkSessionType::Host) {
				// Broadcast to other connections
				if (header.type == NetworkSessionMessageType::ToPeers) {
					// Verify client id
					if (header.srcPeerId != peerId) {
						closeConnection(peerId, "Player sent an invalid srcPlayer");
					} else {
						auto out = makeOutbound(packet.getBytes(), header);
						for (size_t j = 0; j < connections.size(); ++j) {
							if (i != j) {
								connections[j]->send(OutboundNetworkPacket(out));
							}
						}

						// Consume!
						return true;
					}
				} else if (header.type == NetworkSessionMessageType::Control) {
					// Receive control
					ControlMsgHeader controlHeader;
					packet.extractHeader(controlHeader);
					receiveControlMessage(peerId, packet.getBytes());
				} else if (header.type == NetworkSessionMessageType::ToMaster) {
					// For me only
					// Consume!
					return true;
				} else {
					closeConnection(peerId, "Unknown session message type: " + toString(type));
				}
			}

			else if (type == NetworkSessionType::Client) {
				if (header.type == NetworkSessionMessageType::ToPeers) {
					// Consume!
					return true;
				} else if (header.type == NetworkSessionMessageType::Control) {
					receiveControlMessage(peerId, packet.getBytes());
				} else {
					closeConnection(peerId, "Invalid session message type for client: " + toString(type));
				}
			}

			else {
				throw Exception("NetworkSession in invalid state.");
			}
		}
	}
	return false;
}

void NetworkSession::receiveControlMessage(int peerId, gsl::span<const gsl::byte> data)
{

}

void NetworkSession::closeConnection(int peerId, const String& reason)
{
	int connId = type == NetworkSessionType::Host ? peerId - 1 : 0;
	connections.at(connId)->close();
}
