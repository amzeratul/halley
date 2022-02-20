#include "session/network_session.h"

#include <cassert>

#include "session/network_session_control_messages.h"
#include "connection/network_service.h"
#include "connection/network_packet.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
using namespace Halley;

NetworkSession::NetworkSession(NetworkService& service, uint32_t networkVersion, String userName, ISharedDataHandler* sharedDataHandler)
	: service(service)
	, sharedDataHandler(sharedDataHandler)
	, networkVersion(networkVersion)
	, userName(std::move(userName))
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
	hostAddress = service.startListening([=](NetworkService::Acceptor& a) { onConnection(a); });

	setMyPeerId(0);
}

void NetworkSession::join(const String& address)
{
	Expects(type == NetworkSessionType::Undefined);

	type = NetworkSessionType::Client;
	peers.emplace_back(Peer{ 0, true, service.connect(address) });

	auto& conn = *peers.back().connection;
	ControlMsgJoin msg;
	msg.networkVersion = networkVersion;
	msg.userName = userName;
	Bytes bytes = Serializer::toBytes(msg);
	conn.send(IConnection::TransmissionType::Reliable, doMakeControlPacket(NetworkSessionControlMessageType::Join, OutboundNetworkPacket(bytes)));
	
	for (auto* listener : listeners) {
		listener->onPeerConnected(0);
	}
	hostAddress = address;
}

void NetworkSession::acceptConnection(std::shared_ptr<IConnection> incoming)
{
	const auto id = allocatePeerId();
	if (!id) {
		throw Exception("Unable to allocate peer id for incoming connection.", HalleyExceptions::Network);
	}
	
	peers.emplace_back(Peer{ id.value(), true, std::move(incoming) });
}

void NetworkSession::close()
{
	for (auto& peer: peers) {
		disconnectPeer(peer);
	}
	peers.clear();

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

void NetworkSession::update(Time t)
{
	service.update(t);

	// Remove dead connections
	for (auto& peer: peers) {
		if (peer.connection->getStatus() == ConnectionStatus::Closed) {
			disconnectPeer(peer);
		}
	}
	std_ex::erase_if(peers, [] (const Peer& peer) { return !peer.alive; });
	
	// Check for data that needs to be sent
	if (type == NetworkSessionType::Host) {
		checkForOutboundStateChanges(t, {});
	}
	if (type == NetworkSessionType::Host || type == NetworkSessionType::Client) {
		if (myPeerId) {
			auto iter = sharedData.find(myPeerId.value());
			if (iter != sharedData.end()) {
				checkForOutboundStateChanges(t, myPeerId.value());
			}
		}
	}

	// Close if connection is lost
	if (type == NetworkSessionType::Client) {
		if (peers.empty()) {
			close();
		}
	}

	// Update again to dispatch anything
	processReceive();
	service.update(0.0);
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

std::unique_ptr<SharedData> NetworkSession::makeSessionSharedData()
{
	if (sharedDataHandler) {
		return sharedDataHandler->makeSessionSharedData();
	}
	return std::make_unique<SharedData>();
}

std::unique_ptr<SharedData> NetworkSession::makePeerSharedData()
{
	if (sharedDataHandler) {
		return sharedDataHandler->makePeerSharedData();
	}
	return std::make_unique<SharedData>();
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

void NetworkSession::doSendToAll(OutboundNetworkPacket packet, std::optional<PeerId> except)
{
	for (size_t i = 0; i < peers.size(); ++i) {
		if (peers[i].peerId != except) {
			peers[i].connection->send(IConnection::TransmissionType::Reliable, OutboundNetworkPacket(packet));
		}
	}
}

void NetworkSession::sendToPeers(OutboundNetworkPacket packet, std::optional<PeerId> except)
{
	NetworkSessionMessageHeader header;
	header.type = NetworkSessionMessageType::ToAllPeers;
	header.srcPeerId = myPeerId.value();
	header.dstPeerId = 0;

	doSendToAll(makeOutbound(packet.getBytes(), header), except);
}

void NetworkSession::sendToPeer(OutboundNetworkPacket packet, PeerId peerId)
{
	NetworkSessionMessageHeader header;
	header.type = NetworkSessionMessageType::ToPeer;
	header.srcPeerId = myPeerId.value();
	header.dstPeerId = peerId;
	packet.addHeader(header);

	for (size_t i = 0; i < peers.size(); ++i) {
		if (peers[i].peerId == peerId) {
			peers[i].connection->send(IConnection::TransmissionType::Reliable, std::move(packet));
			return;
		}
	}

	// Redirect via host
	for (size_t i = 0; i < peers.size(); ++i) {
		if (peers[i].peerId == 0) {
			peers[i].connection->send(IConnection::TransmissionType::Reliable, std::move(packet));
			return;
		}
	}
	
	Logger::logError("Unable to send message to peer " + toString(static_cast<int>(peerId)) + ": id not found.");
}

std::optional<std::pair<NetworkSession::PeerId, InboundNetworkPacket>> NetworkSession::receive()
{
	if (!inbox.empty()) {
		auto result = std::move(inbox[0]);
		inbox.erase(inbox.begin());
		return result;
	}
	return {};
}

void NetworkSession::addListener(IListener* listener)
{
	if (!std_ex::contains(listeners, listener)) {
		listeners.push_back(listener);
	}
}

void NetworkSession::removeListener(IListener* listener)
{
	std_ex::erase(listeners, listener);
}

void NetworkSession::setSharedDataHandler(ISharedDataHandler* handler)
{
	sharedDataHandler = handler;
}

const String& NetworkSession::getHostAddress() const
{
	return hostAddress;
}

NetworkService& NetworkSession::getService() const
{
	return service;
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
				if (header.type == NetworkSessionMessageType::ToAllPeers) {
					// Verify client id
					if (header.srcPeerId != peerId) {
						closeConnection(peerId, "Player sent an invalid srcPlayer");
					} else {
						doSendToAll(makeOutbound(packet.getBytes(), header), peerId);
						inbox.emplace_back(header.srcPeerId, std::move(packet));
					}
				} else if (header.type == NetworkSessionMessageType::Control) {
					// Receive control
					receiveControlMessage(peerId, packet);
				} else if (header.type == NetworkSessionMessageType::ToPeer) {
					if (header.dstPeerId == myPeerId) {
						inbox.emplace_back(header.srcPeerId, std::move(packet));
					} else {
						// Redirect!
						sendToPeer(makeOutbound(packet.getBytes(), header), header.dstPeerId);
					}
				} else {
					closeConnection(peerId, "Unknown session message type: " + toString(type));
				}
			}

			else if (type == NetworkSessionType::Client) {
				if (header.type == NetworkSessionMessageType::ToAllPeers) {
					inbox.emplace_back(header.srcPeerId, std::move(packet));
				} else if (header.type == NetworkSessionMessageType::ToPeer) {
					if (header.dstPeerId == myPeerId) {
						inbox.emplace_back(header.srcPeerId, std::move(packet));
					} else {
						closeConnection(peerId, "Received message bound for a different client, aborting connection.");
					}
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
	Logger::logError("Closing connection: " + reason);
	for (auto& p: peers) {
		if (p.peerId == peerId) {
			disconnectPeer(p);
		}
	}
}

void NetworkSession::retransmitControlMessage(PeerId peerId, gsl::span<const gsl::byte> bytes)
{
	NetworkSessionMessageHeader header;
	header.type = NetworkSessionMessageType::Control;
	header.srcPeerId = peerId; // ?
	header.dstPeerId = 0;
	doSendToAll(makeOutbound(bytes, header), peerId);
}

void NetworkSession::receiveControlMessage(PeerId peerId, InboundNetworkPacket& packet)
{
	auto origData = packet.getBytes();

	ControlMsgHeader header;
	packet.extractHeader(header);

	switch (header.type) {
	case NetworkSessionControlMessageType::Join:
		{
			ControlMsgJoin msg = Deserializer::fromBytes<ControlMsgJoin>(packet.getBytes());
			onControlMessage(peerId, msg);
		}
		break;
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

void NetworkSession::onControlMessage(PeerId peerId, const ControlMsgJoin& msg)
{
	Logger::logDev("Join request from peer " + toString(int(peerId)));
	
	if (myPeerId != 0) {
		closeConnection(peerId, "Only host can accept join requests.");
		return;
	}

	if (msg.networkVersion != networkVersion) {
		closeConnection(peerId, "Incompatible network version.");
		return;
	}

	ControlMsgSetPeerId outMsg;
	outMsg.peerId = peerId;
	Bytes bytes = Serializer::toBytes(outMsg);
	sharedData[outMsg.peerId] = makePeerSharedData();

	auto& conn = *getPeer(peerId).connection;
	conn.send(IConnection::TransmissionType::Reliable, doMakeControlPacket(NetworkSessionControlMessageType::SetPeerId, OutboundNetworkPacket(bytes)));
	conn.send(IConnection::TransmissionType::Reliable, makeUpdateSharedDataPacket({}));
	for (auto& i : sharedData) {
		conn.send(IConnection::TransmissionType::Reliable, makeUpdateSharedDataPacket(i.first));
	}
	for (auto* listener : listeners) {
		listener->onPeerConnected(peerId);
	}
}

void NetworkSession::onControlMessage(PeerId peerId, const ControlMsgSetPeerId& msg)
{
	if (peerId != 0) {
		closeConnection(peerId, "Unauthorised control message: SetPeerId");
		return;
	}
	setMyPeerId(msg.peerId);
}

void NetworkSession::onControlMessage(PeerId peerId, const ControlMsgSetPeerState& msg)
{
	if (peerId != 0 && peerId != msg.peerId) {
		closeConnection(peerId, "Unauthorised control message: SetPeerState");
		return;
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
		return;
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

	for (auto* listener: listeners) {
		listener->onStartSession(id);
	}
}

NetworkSession::Peer& NetworkSession::getPeer(PeerId id)
{
	return *std::find_if(peers.begin(), peers.end(), [&](const Peer& peer) { return peer.peerId == id; });
}

void NetworkSession::checkForOutboundStateChanges(Time t, std::optional<PeerId> ownerId)
{
	SharedData& data = !ownerId ? *sessionSharedData : *sharedData.at(ownerId.value());
	data.update(t);
	if (data.isModified()) {
		doSendToAll(makeUpdateSharedDataPacket(ownerId), {});
		data.markUnmodified();
		data.markSent();
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
	header.srcPeerId = myPeerId ? myPeerId.value() : 0;
	header.dstPeerId = 0;
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

	auto avail = Vector<uint8_t>(maxClients, 1);
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

void NetworkSession::disconnectPeer(Peer& peer)
{
	if (peer.connection->getStatus() != ConnectionStatus::Closed) {
		peer.connection->close();
	}
	if (peer.alive) {
		for (auto* listener : listeners) {
			listener->onPeerDisconnected(peer.peerId);
		}
		peer.alive = false;
	}
}
