#include "session/network_session.h"

#include <cassert>

#include "session/network_session_control_messages.h"
#include "connection/network_service.h"
#include "connection/network_packet.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

NetworkSession::NetworkSession(NetworkService& service)
	: service(service)
{
}

NetworkSession::~NetworkSession()
{
	if (type == NetworkSessionType::Host) {
		service.stopListening();
	}
	close();
}

void NetworkSession::host(uint16_t maxClients)
{
	Expects(type == NetworkSessionType::Undefined);

	this->maxClients = maxClients;
	type = NetworkSessionType::Host;
	sessionSharedData = makeSessionSharedData();
	service.startListening([=](NetworkService::Acceptor& a) { onConnection(a); });

	onStartSession();
	setMyPeerId(0);
	onHosting();
}

void NetworkSession::join(const String& address)
{
	Expects(type == NetworkSessionType::Undefined);

	peers.emplace_back(Peer{ 0, service.connect(address) });
	
	type = NetworkSessionType::Client;

	onStartSession();
}

void NetworkSession::close()
{
	for (auto& peer: peers) {
		peer.connection->close();
		onDisconnected(peer.peerId);
	}
	peers.clear();

	type = NetworkSessionType::Undefined;
	myPeerId = {};
}

void NetworkSession::setMaxClients(uint16_t clients)
{
	maxClients = clients;
}

uint16_t NetworkSession::getMaxClients() const
{
	return maxClients;
}

std::optional<NetworkSession::PeerId> NetworkSession::getMyPeerId() const
{
	return myPeerId;
}

uint16_t NetworkSession::getClientCount() const
{
	if (type == NetworkSessionType::Client) {
		throw Exception("Client shouldn't be trying to query client count!", HalleyExceptions::Network);
		//return getStatus() != ConnectionStatus::Open ? 0 : 2; // TODO
	} else if (type == NetworkSessionType::Host) {
		uint16_t i = 1;
		for (auto& peer: peers) {
			if (peer.connection->getStatus() == ConnectionStatus::Connected) {
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
	const auto id = allocatePeerId();
	if (!id) {
		throw Exception("Unable to allocate peer id for incoming connection.", HalleyExceptions::Network);
	}
	
	auto& peer = peers.emplace_back(Peer{ id.value(), std::move(incoming) });

	ControlMsgSetPeerId msg;
	msg.peerId = peer.peerId;
	Bytes bytes = Serializer::toBytes(msg);
	sharedData[msg.peerId] = makePeerSharedData();

	auto& conn = *peer.connection;
	conn.send(doMakeControlPacket(NetworkSessionControlMessageType::SetPeerId, OutboundNetworkPacket(bytes)));
	conn.send(makeUpdateSharedDataPacket({}));
	for (auto& i: sharedData) {
		conn.send(makeUpdateSharedDataPacket(i.first));
	}
	onConnected(peer.peerId);
}

void NetworkSession::update()
{
	// Remove dead connections
	service.update();
	for (auto& peer: peers) {
		if (peer.connection->getStatus() == ConnectionStatus::Closed) {
			onDisconnected(peer.peerId);
		}
	}
	std_ex::erase_if(peers, [] (const Peer& peer)
	{
		return peer.connection->getStatus() == ConnectionStatus::Closed;
	});

	if (type == NetworkSessionType::Host) {
		checkForOutboundStateChanges({});
	}

	if (type == NetworkSessionType::Client) {
		if (peers.empty()) {
			close();
		}
	}

	if (type == NetworkSessionType::Host || type == NetworkSessionType::Client) {
		if (myPeerId) {
			auto iter = sharedData.find(myPeerId.value());
			if (iter != sharedData.end()) {
				checkForOutboundStateChanges(myPeerId.value());
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
	if (type == NetworkSessionType::Undefined || !myPeerId) {
		throw Exception("Not connected.", HalleyExceptions::Network);
	}
	auto iter = sharedData.find(myPeerId.value());
	if (iter == sharedData.end()) {
		throw Exception("Not connected.", HalleyExceptions::Network);
	}
	return *iter->second;
}

SharedData& NetworkSession::doGetMutableSessionSharedData()
{
	if (type != NetworkSessionType::Host) {
		throw Exception("Only the host can modify shared session data.", HalleyExceptions::Network);
	}
	return *sessionSharedData;
}

const SharedData& NetworkSession::doGetSessionSharedData() const
{
	return *sessionSharedData;
}

const SharedData& NetworkSession::doGetClientSharedData(PeerId clientId) const
{
	const auto result = doTryGetClientSharedData(clientId);
	if (!result) {
		throw Exception("Unknown client with id: " + toString(static_cast<int>(clientId)), HalleyExceptions::Network);
	}
	return *result;
}

const SharedData* NetworkSession::doTryGetClientSharedData(PeerId clientId) const
{
	const auto iter = sharedData.find(clientId);
	if (iter == sharedData.end()) {
		return nullptr;
	}
	return iter->second.get();
}

void NetworkSession::onStartSession()
{
}

void NetworkSession::onPeerIdAssigned()
{
}

void NetworkSession::onHosting()
{
}

void NetworkSession::onConnected(PeerId peerId)
{
	Logger::logDev("Peer connected to network: " + toString(static_cast<int>(peerId)));
}

void NetworkSession::onDisconnected(PeerId peerId)
{
	Logger::logDev("Peer disconnected from network: " + toString(static_cast<int>(peerId)));
}

ConnectionStatus NetworkSession::getStatus() const
{
	if (type == NetworkSessionType::Undefined) {
		return ConnectionStatus::Undefined;
	} else if (type == NetworkSessionType::Client) {
		if (peers.empty()) {
			return ConnectionStatus::Closed;
		} else {
			if (peers[0].connection->getStatus() == ConnectionStatus::Connected) {
				return myPeerId && sessionSharedData ? ConnectionStatus::Connected : ConnectionStatus::Connecting;
			} else {
				return peers[0].connection->getStatus();
			}
		}
	} else if (type == NetworkSessionType::Host) {
		return ConnectionStatus::Connected;
	} else {
		throw Exception("Unknown session type.", HalleyExceptions::Network);
	}
}

OutboundNetworkPacket NetworkSession::makeOutbound(gsl::span<const gsl::byte> data, NetworkSessionMessageHeader header)
{
	auto packet = OutboundNetworkPacket(data);
	packet.addHeader(header);
	return packet;
}

void NetworkSession::sendToAll(OutboundNetworkPacket packet, int except)
{
	for (size_t i = 0; i < peers.size(); ++i) {
		if (peers[i].peerId != except) {
			peers[i].connection->send(OutboundNetworkPacket(packet));
		}
	}
}

void NetworkSession::send(OutboundNetworkPacket packet)
{
	NetworkSessionMessageHeader header;
	header.type = NetworkSessionMessageType::ToPeers;
	header.srcPeerId = myPeerId.value();

	auto out = makeOutbound(packet.getBytes(), header);
	for (auto& peer: peers) {
		peer.connection->send(OutboundNetworkPacket(out));
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
	for (size_t i = 0; i < peers.size(); ++i) {
		const bool gotMessage = peers[i].connection->receive(packet);
		if (gotMessage) {
			// Get header
			const PeerId peerId = peers[i].peerId;
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
				if (header.type == NetworkSessionMessageType::ToPeers) {
					// Consume!
					inbox.emplace_back(std::move(packet));
				} else if (header.type == NetworkSessionMessageType::Control) {
					receiveControlMessage(peerId, packet);
				} else {
					closeConnection(peerId, "Invalid session message type for client: " + toString(type));
				}
			}

			else {
				throw Exception("NetworkSession in invalid state.", HalleyExceptions::Network);
			}
		}
	}
}

void NetworkSession::closeConnection(PeerId peerId, const String& reason)
{
	for (auto& p: peers) {
		if (p.peerId == peerId) {
			onDisconnected(p.peerId);
			p.connection->close();
		}
	}
}

void NetworkSession::retransmitControlMessage(PeerId peerId, gsl::span<const gsl::byte> bytes)
{
	NetworkSessionMessageHeader header;
	header.type = NetworkSessionMessageType::Control;
	header.srcPeerId = peerId;
	sendToAll(makeOutbound(bytes, header), peerId);
}

void NetworkSession::receiveControlMessage(PeerId peerId, InboundNetworkPacket& packet)
{
	auto origData = packet.getBytes();

	ControlMsgHeader header;
	packet.extractHeader(header);

	switch (header.type) {
	case NetworkSessionControlMessageType::SetPeerId:
		{
			ControlMsgSetPeerId msg = Deserializer::fromBytes<ControlMsgSetPeerId>(packet.getBytes());
			onControlMessage(peerId, msg);
		}
		break;
	case NetworkSessionControlMessageType::SetSessionState:
		{
			ControlMsgSetSessionState msg = Deserializer::fromBytes<ControlMsgSetSessionState>(packet.getBytes());
			onControlMessage(peerId, msg);
		}
		break;
	case NetworkSessionControlMessageType::SetPeerState:
		{
			ControlMsgSetPeerState msg = Deserializer::fromBytes<ControlMsgSetPeerState>(packet.getBytes());
			onControlMessage(peerId, msg);
			retransmitControlMessage(peerId, origData);
		}
		break;
	default:
		closeConnection(peerId, "Invalid control packet.");
	}
}

void NetworkSession::onControlMessage(PeerId peerId, const ControlMsgSetPeerId& msg)
{
	if (peerId != 0) {
		closeConnection(peerId, "Unauthorised control message: SetPeerId");
	}
	setMyPeerId(msg.peerId);
}

void NetworkSession::onControlMessage(PeerId peerId, const ControlMsgSetPeerState& msg)
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

void NetworkSession::onControlMessage(PeerId peerId, const ControlMsgSetSessionState& msg)
{
	if (peerId != 0) {
		closeConnection(peerId, "Unauthorised control message: SetSessionState");
	}

	if (!sessionSharedData) {
		sessionSharedData = makeSessionSharedData();
	}
	auto s = Deserializer(msg.state);
	sessionSharedData->deserialize(s);
}

void NetworkSession::setMyPeerId(PeerId id)
{
	Expects (!myPeerId);
	myPeerId = id;
	sharedData[id] = makePeerSharedData();

	onPeerIdAssigned();
}

void NetworkSession::checkForOutboundStateChanges(std::optional<PeerId> ownerId)
{
	SharedData& data = !ownerId ? *sessionSharedData : *sharedData.at(ownerId.value());
	if (data.isModified()) {
		sendToAll(makeUpdateSharedDataPacket(ownerId));
		data.markUnmodified();
	}
}

OutboundNetworkPacket NetworkSession::makeUpdateSharedDataPacket(std::optional<PeerId> ownerId)
{
	SharedData& data = !ownerId ? *sessionSharedData : *sharedData.at(ownerId.value());
	if (!ownerId) {
		ControlMsgSetSessionState state;
		state.state = Serializer::toBytes(data);
		Bytes bytes = Serializer::toBytes(state);
		return doMakeControlPacket(NetworkSessionControlMessageType::SetSessionState, OutboundNetworkPacket(bytes));
	} else {
		ControlMsgSetPeerState state;
		state.peerId = ownerId.value();
		state.state = Serializer::toBytes(data);
		Bytes bytes = Serializer::toBytes(state);
		return doMakeControlPacket(NetworkSessionControlMessageType::SetPeerState, OutboundNetworkPacket(bytes));
	}
}

OutboundNetworkPacket NetworkSession::doMakeControlPacket(NetworkSessionControlMessageType msgType, OutboundNetworkPacket packet)
{
	ControlMsgHeader ctrlHeader;
	ctrlHeader.type = msgType;
	packet.addHeader(ctrlHeader);

	NetworkSessionMessageHeader header;
	header.type = NetworkSessionMessageType::Control;
	header.srcPeerId = myPeerId.value();
	packet.addHeader(header);

	return packet;
}

void NetworkSession::onConnection(NetworkService::Acceptor& acceptor)
{
	if (getClientCount() < maxClients) { // I'm also a client!
		acceptConnection(acceptor.accept());
	} else {
		Logger::logInfo("Rejecting network session connection as we're already at max clients.");
		acceptor.reject();
	}
}

std::optional<NetworkSession::PeerId> NetworkSession::allocatePeerId() const
{
	Expects(type == NetworkSessionType::Host);

	auto avail = std::vector<uint8_t>(maxClients, 1);
	avail[0] = 0;
	for (const auto& p: peers) {
		assert(avail.at(p.peerId) == 1);
		avail.at(p.peerId) = 0;
	}

	for (uint8_t i = 1; i < avail.size(); ++i) {
		if (avail[i] != 0) {
			return i;
		}
	}
	return {};
}
