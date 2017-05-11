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
	setMyPeerId(0);
}

void NetworkSession::join(const String& address, int port)
{
	Expects(type == NetworkSessionType::Undefined);

	connections.emplace_back(service.connect(address, port));
	
	type = NetworkSessionType::Client;
	setMyPeerId(1);
}

void NetworkSession::close()
{
	for (auto& c: connections) {
		c->close();
	}
	connections.clear();

	type = NetworkSessionType::Undefined;
	myPeerId = -1;
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

SharedData& NetworkSession::doGetMySharedData()
{
	if (type == NetworkSessionType::Undefined || myPeerId == -1) {
		throw Exception("Not connected.");
	}
	auto iter = sharedData.find(myPeerId);
	if (iter == sharedData.end()) {
		throw Exception("Not connected.");
	}
	return *iter->second;
}

SharedData& NetworkSession::doGetMutableSessionSharedData()
{
	if (type != NetworkSessionType::Host) {
		throw Exception("Only the host can modify shared session data.");
	}
	return *sessionSharedData;
}

const SharedData& NetworkSession::doGetSessionSharedData() const
{
	return *sessionSharedData;
}

const SharedData& NetworkSession::doGetClientSharedData(int clientId) const
{
	auto iter = sharedData.find(clientId);
	if (iter == sharedData.end()) {
		throw Exception("Unknown client with id: " + toString(clientId));
	}
	return *iter->second;
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
	header.srcPeerId = myPeerId;

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
					receiveControlMessage(peerId, packet);
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
					receiveControlMessage(peerId, packet);
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

void NetworkSession::closeConnection(int peerId, const String& reason)
{
	int connId = type == NetworkSessionType::Host ? peerId - 1 : 0;
	connections.at(connId)->close();
	std::cout << "Terminating connection with peer " << peerId << " with message: " << reason << std::endl;
}

void NetworkSession::retransmitControlMessage(int peerId, gsl::span<const gsl::byte> bytes)
{
	NetworkSessionMessageHeader header;
	header.type = NetworkSessionMessageType::Control;
	header.srcPeerId = peerId;
	auto out = makeOutbound(bytes, header);
	for (size_t i = 0; i < connections.size(); ++i) {
		if (i != peerId) {
			connections[i]->send(OutboundNetworkPacket(out));
		}
	}
}

void NetworkSession::receiveControlMessage(int peerId, InboundNetworkPacket& packet)
{
	auto origData = packet.getBytes();

	ControlMsgHeader header;
	packet.extractHeader(header);
	switch (header.type) {
	case NetworkSessionControlMessageType::SetPeerId:
		{
			ControlMsgSetPeerId msg;
			packet.extractHeader(msg);
			onControlMessage(peerId, msg);
		}
		break;
	case NetworkSessionControlMessageType::SetSessionState:
		{
			ControlMsgSetSessionState msg;
			packet.extractHeader(msg);
			onControlMessage(peerId, msg);
		}
		break;
	case NetworkSessionControlMessageType::SetPeerState:
		{
			ControlMsgSetPeerState msg;
			packet.extractHeader(msg);
			onControlMessage(peerId, msg);
			retransmitControlMessage(peerId, origData);
		}
		break;
	default:
		closeConnection(peerId, "Invalid control packet.");
	}
}

void NetworkSession::onControlMessage(int peerId, const ControlMsgSetPeerId& msg)
{
	if (peerId != 0) {
		closeConnection(peerId, "Unauthorised control message: SetPeerId");
	}
	setMyPeerId(msg.peerId);
}

void NetworkSession::onControlMessage(int peerId, const ControlMsgSetPeerState& msg)
{
	if (peerId != 0 && peerId != msg.peerId) {
		closeConnection(peerId, "Unauthorised control message: SetPeerState");
	}
	auto iter = sharedData.find(msg.peerId);

	auto s = Deserializer(msg.state);
	if (iter != sharedData.end()) {
		iter->second->deserialize(s);
	} else {
		sharedData[msg.peerId] = makePeerSharedData();
		sharedData[msg.peerId]->deserialize(s);
	}
}

void NetworkSession::onControlMessage(int peerId, const ControlMsgSetSessionState& msg)
{
	if (peerId != 0) {
		closeConnection(peerId, "Unauthorised control message: SetSessionState");
	}

	auto s = Deserializer(msg.state);
	sessionSharedData->deserialize(s);
}

void NetworkSession::setMyPeerId(int id)
{
	myPeerId = id;
	sessionSharedData = makeSessionSharedData();
}
