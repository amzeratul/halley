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
		return getStatus() != ConnectionStatus::Open ? 0 : 2; // TODO
	} else if (type == NetworkSessionType::Host) {
		int i = 1;
		for (auto& c: connections) {
			if (c->getStatus() == ConnectionStatus::Open) {
				++i;
			}
		}
		return i;
	} else {
		return 0;
	}
}

void NetworkSession::acceptConnection(std::shared_ptr<IConnection> incoming)
{
	connections.emplace_back(std::move(incoming));

	ControlMsgSetPeerId msg;
	msg.peerId = int8_t(connections.size());
	connections.back()->send(makeControlPacket(NetworkSessionControlMessageType::SetPeerId, msg));
}

void NetworkSession::update()
{
	// Remove dead connections
	service.update();
	connections.erase(std::remove_if(connections.begin(), connections.end(), [] (const std::shared_ptr<IConnection>& c) { return c->getStatus() == ConnectionStatus::Closed; }), connections.end());

	if (type == NetworkSessionType::Host) {
		if (getClientCount() < maxClients) { // I'm also a client!
			service.setAcceptingConnections(true);
			auto incoming = service.tryAcceptConnection();
			if (incoming) {
				acceptConnection(std::move(incoming));
			}
		} else {
			service.setAcceptingConnections(false);
		}

		checkForOutboundStateChanges(true, *sessionSharedData);
	}

	if (type == NetworkSessionType::Client) {
		if (connections.empty()) {
			close();
		}
	}

	if (type == NetworkSessionType::Host || type == NetworkSessionType::Client) {
		if (myPeerId != -1) {
			auto iter = sharedData.find(myPeerId);
			if (iter != sharedData.end()) {
				checkForOutboundStateChanges(false, *iter->second);
			}
		}
	}

	// Update again to dispatch anything
	processReceive();
	service.update();
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
		return ConnectionStatus::Undefined;
	} else if (type == NetworkSessionType::Client) {
		if (connections.empty()) {
			return ConnectionStatus::Closed;
		} else {
			return connections[0]->getStatus() == ConnectionStatus::Open ? (myPeerId != -1 ? ConnectionStatus::Open : ConnectionStatus::Connecting) : connections[0]->getStatus();
		}
	} else if (type == NetworkSessionType::Host) {
		return ConnectionStatus::Open;
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

void NetworkSession::sendToAll(OutboundNetworkPacket&& packet, int except)
{
	for (size_t i = 0; i < connections.size(); ++i) {
		if (int(i) != except) {
			connections[i]->send(OutboundNetworkPacket(packet));
		}
	}
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
	if (!inbox.empty()) {
		packet = InboundNetworkPacket(std::move(inbox[0]));
		inbox.erase(inbox.begin());
		return true;
	}
	return false;
}

void NetworkSession::processReceive()
{
	InboundNetworkPacket packet;
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
						sendToAll(makeOutbound(packet.getBytes(), header), int(i));
						inbox.emplace_back(std::move(packet));
					}
				} else if (header.type == NetworkSessionMessageType::Control) {
					// Receive control
					ControlMsgHeader controlHeader;
					packet.extractHeader(controlHeader);
					receiveControlMessage(peerId, packet);
				} else if (header.type == NetworkSessionMessageType::ToMaster) {
					// For me only
					// Consume!
					inbox.emplace_back(std::move(packet));
				} else {
					closeConnection(peerId, "Unknown session message type: " + toString(type));
				}
			}

			else if (type == NetworkSessionType::Client) {
				std::cout << "Got msg.\n";
				if (header.type == NetworkSessionMessageType::ToPeers) {
					// Consume!
					inbox.emplace_back(std::move(packet));
				} else if (header.type == NetworkSessionMessageType::Control) {
					std::cout << "Got ctrl msg.\n";
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
	sendToAll(makeOutbound(bytes, header), peerId);
}

void NetworkSession::receiveControlMessage(int peerId, InboundNetworkPacket& packet)
{
	auto origData = packet.getBytes();

	ControlMsgHeader header;
	packet.extractHeader(header);

	std::cout << "Ctrl msg type is " << int(header.type) << ".\n";

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
	std::cout << "Received session state." << std::endl;
}

void NetworkSession::setMyPeerId(int id)
{
	std::cout << "Setting my peer id to " << id << std::endl;
	myPeerId = id;
	sessionSharedData = makeSessionSharedData();
}

void NetworkSession::checkForOutboundStateChanges(bool isSessionState, SharedData& data)
{
	if (data.isModified()) {
		std::cout << "Sending shared data..." << std::endl;

		if (isSessionState) {
			ControlMsgSetSessionState state;
			state.state = Serializer::toBytes(data);
			sendToAll(makeControlPacket(NetworkSessionControlMessageType::SetSessionState, state));
		} else {
			ControlMsgSetPeerState state;
			state.peerId = myPeerId;
			state.state = Serializer::toBytes(data);
			sendToAll(makeControlPacket(NetworkSessionControlMessageType::SetPeerState, state));
		}

		data.markUnmodified();
	}
}

OutboundNetworkPacket NetworkSession::doMakeControlPacket(NetworkSessionControlMessageType msgType, OutboundNetworkPacket&& packet)
{
	NetworkSessionMessageHeader header;
	header.type = NetworkSessionMessageType::Control;
	header.srcPeerId = myPeerId;

	ControlMsgHeader ctrlHeader;
	ctrlHeader.type = msgType;

	packet.addHeader(ctrlHeader);

	packet.addHeader(header);

	return packet;
}
