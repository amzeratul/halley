#include "halley/net/entity/entity_network_session.h"

#include <cassert>

#include "halley/bytes/compression.h"
#include "halley/net/interpolators/data_interpolator.h"
#include "halley/net/interpolators/byte_data_interpolator.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/system.h"
#include "halley/entity/world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "components/network_component.h"

class NetworkComponent;
using namespace Halley;

static constexpr size_t receiveBufferSize = 32 * 1024;
static constexpr size_t maxCompressedMessageSize = 16000;
static constexpr size_t maxLargeMessageSplitCount = 16;

EntityNetworkSession::EntityNetworkSession(std::shared_ptr<NetworkSession> session, Resources& resources, HashSet<String> ignoreComponents, IEntityNetworkSessionListener* listener)
	: resources(resources)
	, listener(listener)
	, session(std::move(session))
{
	Expects(this->session);
	Expects(this->listener);
	
	this->session->addListener(this);

	entitySerializationOptions.type = EntitySerialization::Type::Network;

	deltaOptions.preserveComponentOrder = false;
	deltaOptions.shallow = false;
	deltaOptions.deltaComponents = true;
	deltaOptions.allowNonSerializable = false;
	deltaOptions.omitEmptyComponents = true;
	deltaOptions.ignoreComponents = std::move(ignoreComponents);

	setupDictionary();
	byteSerializationOptions.version = SerializerOptions::maxVersion;
	byteSerializationOptions.dictionary = &serializationDictionary;

	receiveBuffer.resize(receiveBufferSize);
}

EntityNetworkSession::~EntityNetworkSession()
{
	session->removeListener(this);
}

void EntityNetworkSession::setWorld(World& world, SystemMessageBridge bridge)
{
	factory = std::make_shared<EntityFactory>(world, resources);
	factory->setNetworkFactory(true);
	messageBridge = bridge;

	// Clear queue
	if (!queuedPackets.empty()) {
		for (auto& qp: queuedPackets) {
			processMessage(qp.fromPeerId, std::move(qp.message));
		}
		queuedPackets.clear();
	}
}

void EntityNetworkSession::update(Time t)
{
	session->update(t);

	for (auto& peer: peers) {
		peer.update(t);
	}
}

void EntityNetworkSession::sendUpdates()
{
	sendMessages();
}

void EntityNetworkSession::sendEntityUpdates(Time t, Rect4i viewRect, uint8_t myPeerId, gsl::span<const EntityNetworkUpdateInfo> entityIds)
{
	// Update viewport
	auto& data = session->getMySharedData<EntityClientSharedData>();
	if (data.viewRect != viewRect) {
		const bool first = !data.viewRect;
		data.viewRect = viewRect;
		if (first || data.getTimeSinceLastSend() > 0.05) {
			data.markModified();
		}
	}

	// Update entities
    Vector<Future<void>> tasks;

    for (auto& peer : peers) {
        tasks += Concurrent::execute([&]() {
            peer.sendEntities(t, myPeerId, entityIds, session->getClientSharedData<EntityClientSharedData>(peer.getPeerId()));
        });
    }

    Concurrent::whenAll(tasks.begin(), tasks.end()).wait();
}

void EntityNetworkSession::sendToAll(EntityNetworkMessage msg)
{
	outbox[-1].push_back(std::move(msg));
}

void EntityNetworkSession::sendToPeer(EntityNetworkMessage msg, NetworkSession::PeerId peerId)
{
	outbox[peerId].push_back(std::move(msg));
}

void EntityNetworkSession::requestLobbyInfo()
{
	if (isHost()) {
		if (listener) {
			listener->onReceiveLobbyInfo(getLobbyInfo());
		}
	} else if (!peers.empty()) {
		peers.back().requestLobbyInfo();
	}
}

void EntityNetworkSession::setLobbyInfo(ConfigNode info)
{
	if (isHost()) {
		if (listener) {
			if (listener->setLobbyInfo(0, info)) {
				sendUpdatedLobbyInfos({});
			}
		}
	} else if (!peers.empty()) {
		peers.back().setLobbyInfo(std::move(info));
	}
}

void EntityNetworkSession::sendMessages()
{
	auto tryCompress = [&](size_t startIdx, size_t count, const Vector<EntityNetworkMessage>& msgs) -> std::optional<Bytes>
	{
		auto data = Serializer::toBytes(msgs.span().subspan(startIdx, count), byteSerializationOptions);
        if (data.size() > receiveBuffer.capacity()) {
            // EntityNetworkSession::receiveUpdates() uses a fixed sized buffer to decompress into.
            // Let's just check the size right here, and split if needed.
            return std::nullopt;
        }
		auto compressed = Compression::lz4Compress(gsl::as_bytes(gsl::span<const Byte>(data)));
		if (compressed.size() <= maxCompressedMessageSize) {
			return std::move(compressed);
		} else {
			// Compressed packet data may be larger than 16x MTU, which currently is a hard
			// limit of AckUnreliableConnection.
			return std::nullopt;
		}
	};

	for (const auto& [peerId, msgs]: outbox) {
		size_t startIdx = 0;
		size_t curCount = msgs.size();

		while (startIdx < msgs.size()) {
			if (auto data = tryCompress(startIdx, curCount, msgs)) {
				auto packet = OutboundNetworkPacket(*data);
				packet.addHeader(static_cast<uint8_t>(0x00));
				if (peerId == -1) {
					session->sendToPeers(std::move(packet));
				} else {
					session->sendToPeer(std::move(packet), static_cast<NetworkSession::PeerId>(peerId));
				}
				startIdx += curCount;
				curCount = msgs.size() - startIdx;
			} else {
				if (curCount > 1) {
					// Has more than one pack, but couldn't fit them - try fitting half.
					// It might be able to fit more, but halving will approach the solution faster
					// than trying to find the exact number, at a cost of a bit of inefficiency.
					curCount /= 2;
				} else {
					// Single packet, too big to send. Split it up into multiple messages, and use
					// the message header to "encode" information for the receiver.
					auto largeData = Serializer::toBytes(msgs.span().subspan(startIdx, 1), byteSerializationOptions);

					const size_t splitSize = receiveBuffer.capacity();
					size_t numSplit = largeData.size() / splitSize;
					if (largeData.size() % splitSize != 0) numSplit++;

					Expects(numSplit < maxLargeMessageSplitCount); // Larger than Nx32 kb. We shouldn't do that!

					for (size_t split = 0, splitOffset = 0; split < numSplit; split++, splitOffset += splitSize) {
						size_t remain = std::min(largeData.size() - splitOffset, splitSize);
						auto subData = largeData.const_byte_span().subspan(splitOffset, remain);
						auto compressed = Compression::lz4Compress(subData);

						Expects(compressed.size() <= maxCompressedMessageSize);

						auto packet = OutboundNetworkPacket(compressed);
						packet.addHeader(static_cast<uint8_t>(numSplit << 4 | split));

						if (peerId == -1) {
							session->sendToPeers(std::move(packet));
						} else {
							session->sendToPeer(std::move(packet), static_cast<NetworkSession::PeerId>(peerId));
						}
					}

					++startIdx;
					curCount = msgs.size() - startIdx;
				}
			}
		}
	}
	outbox.clear();
}

void EntityNetworkSession::receiveUpdates()
{
	session->update(0.0);

	while (auto result = session->receive()) {
		const auto fromPeerId = result->first;
		auto& packet = result->second;

		// Read header to check if this is a regular, or part of a large message.
		uint8_t largeSplitHeader;
		packet.extractHeader(largeSplitHeader);

		receiveBuffer.resize(receiveBuffer.capacity());
		const auto size = Compression::lz4Decompress(packet.getBytes(), gsl::as_writable_bytes(receiveBuffer.span()));
		if (size) {
			receiveBuffer.resize(*size);
		} else {
			Logger::logError("Failed to decompress network packet");
			continue;
		}

		if (largeSplitHeader == 0) {
			auto msgs = Deserializer::fromBytes<Vector<EntityNetworkMessage>>(receiveBuffer, byteSerializationOptions);

			for (auto& msg: msgs) {
				if (canProcessMessage(msg)) {
					processMessage(fromPeerId, std::move(msg));
				} else {
					queuedPackets.emplace_back(QueuedMessage{ fromPeerId, std::move(msg) });
				}
			}
		} else {
			if (!largePacketBuffer.contains(fromPeerId)) {
				// Only reserve on demand, 512 kb right now, per peer that sends large packets.
				largePacketBuffer[fromPeerId] = Bytes(maxLargeMessageSplitCount * receiveBuffer.capacity());
			}

			Bytes& buffer = largePacketBuffer[fromPeerId];

			// NB: the network connection makes sure that messages are received in order, so we
			// (hopefully) don't have to take care of that right here.
			size_t numSplit = largeSplitHeader >> 4;
			size_t curSplit = largeSplitHeader & 15;

			size_t srcSize = receiveBuffer.size();
			size_t dstOffs = buffer.size();

			if (curSplit == 0) {
				// First part, reset the buffer.
				buffer.resize_no_init(0);
				dstOffs = 0;
			}

			// Resize first, then copy.
			buffer.resize_no_init(dstOffs + srcSize);
			auto dst = buffer.byte_span().subspan(dstOffs, srcSize);
			memcpy(dst.data(), receiveBuffer.data(), srcSize);

			if (curSplit == numSplit - 1) {
				// Last part of large message. Can process it now.
				auto msgs = Deserializer::fromBytes<Vector<EntityNetworkMessage>>(buffer, byteSerializationOptions);

				for (auto& msg: msgs) {
					if (canProcessMessage(msg)) {
						processMessage(fromPeerId, std::move(msg));
					} else {
						queuedPackets.emplace_back(QueuedMessage{ fromPeerId, std::move(msg) });
					}
				}
			}
		}
	}
}

bool EntityNetworkSession::canProcessMessage(const EntityNetworkMessage& msg) const
{
	return factory || !msg.needsWorld();
}

void EntityNetworkSession::processMessage(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg)
{
	switch (msg.getType()) {
	case EntityNetworkHeaderType::Create:
	case EntityNetworkHeaderType::Destroy:
	case EntityNetworkHeaderType::Update:
		onReceiveEntityUpdate(fromPeerId, std::move(msg));
		break;
	case EntityNetworkHeaderType::ReadyToStart:
		onReceiveReady(fromPeerId, msg.getMessage<EntityNetworkMessageReadyToStart>());
		break;
	case EntityNetworkHeaderType::EntityMsg:
		onReceiveMessageToEntity(fromPeerId, msg.getMessage<EntityNetworkMessageEntityMsg>());
		break;
	case EntityNetworkHeaderType::SystemMsg:
		onReceiveSystemMessage(fromPeerId, msg.getMessage<EntityNetworkMessageSystemMsg>());
		break;
	case EntityNetworkHeaderType::SystemMsgResponse:
		onReceiveSystemMessageResponse(fromPeerId, msg.getMessage<EntityNetworkMessageSystemMsgResponse>());
		break;
	case EntityNetworkHeaderType::KeepAlive:
		break;
	case EntityNetworkHeaderType::JoinWorld:
		onReceiveJoinWorld(fromPeerId);
		break;
	case EntityNetworkHeaderType::GetLobbyInfo:
		onReceiveGetLobbyInfo(fromPeerId, msg.getMessage<EntityNetworkMessageGetLobbyInfo>());
		break;
	case EntityNetworkHeaderType::UpdateLobbyInfo:
		onReceiveUpdateLobbyInfo(fromPeerId, msg.getMessage<EntityNetworkMessageUpdateLobbyInfo>());
		break;
	case EntityNetworkHeaderType::SetLobbyInfo:
		onReceiveSetLobbyInfo(fromPeerId, msg.getMessage<EntityNetworkMessageSetLobbyInfo>());
		break;
	}
}

void EntityNetworkSession::onReceiveEntityUpdate(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg)
{
	for (auto& peer: peers) {
		if (peer.getPeerId() == fromPeerId) {
			peer.receiveNetworkMessage(fromPeerId, std::move(msg));
			return;
		}
	}
}

void EntityNetworkSession::onReceiveReady(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageReadyToStart& msg)
{
	Logger::logDev("onReceiveReady from " + toString(int(fromPeerId)));
	if (fromPeerId == 0) {
		readyToStartGame = true;
	}
}

void EntityNetworkSession::onReceiveMessageToEntity(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageEntityMsg& msg)
{
	Expects(factory);
	Expects(messageBridge.isValid());

	auto& world = factory->getWorld();

	if (const auto entity = world.findEntity(msg.entityUUID)) {
		messageBridge.sendMessageToEntity(entity->getEntityId(), msg.messageType, gsl::as_bytes(gsl::span<const Byte>(msg.messageData)), fromPeerId);
	} else {
		const auto& messageName = String(world.getReflection().getMessageReflector(msg.messageType).getName());
		Logger::logError("Received message \"" + messageName + "\" for entity " + toString(msg.entityUUID) + ", but entity was not found.");
	}
}

void EntityNetworkSession::sendEntityMessage(EntityRef entity, int messageType, Bytes messageData)
{
	const NetworkSession::PeerId toPeerId = entity.getOwnerPeerId().value();
	sendToPeer(EntityNetworkMessageEntityMsg(entity.getInstanceUUID(), messageType, std::move(messageData)), toPeerId);
}

void EntityNetworkSession::sendSystemMessage(String targetSystem, int messageType, Bytes messageData, SystemMessageDestination destination, SystemMessageCallback callback)
{
	Expects(destination != SystemMessageDestination::Local);

	// Only wait for responses from host
	// In order to support responses from all players, this class needs to track which clients are in the session and probably include timeouts
	const bool wantsResponse = destination == SystemMessageDestination::Host && callback;
	if (!wantsResponse && callback) {
		Logger::logError("Sending System Message " + toString(messageType) + " with a callback, but not sending it to host only, so it won't ever receive a remote response.");
	}

	// Create message
	const auto id = systemMessageId++;
	auto msg = EntityNetworkMessageSystemMsg(messageType, id, wantsResponse, std::move(targetSystem), destination, std::move(messageData));
	if (wantsResponse) {
		pendingSysMsgResponses[id] = PendingSysMsgResponse{ std::move(callback) };
		assert(pendingSysMsgResponses.size() < 1000); // Make sure we're not leaking
	}

	// Send
	if (destination == SystemMessageDestination::Host) {
		assert(!isHost());
		sendToPeer(std::move(msg), 0);
	} else {
		sendToAll(std::move(msg));
	}
}

void EntityNetworkSession::onReceiveSystemMessage(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageSystemMsg& msg)
{
	Expects(factory);
	Expects(messageBridge.isValid());

	const auto msgType = msg.messageType;
	const auto msgId = msg.msgId;

	SystemMessageCallback callback;
	if (msg.wantsResponse) {
		callback = [=](gsl::byte*, Bytes serializedData)
		{
			sendToPeer(EntityNetworkMessageSystemMsgResponse(msgType, msgId, serializedData), fromPeerId);
		};
	}

	messageBridge.sendMessageToSystem(msg.targetSystem, msg.messageType, gsl::as_bytes(gsl::span<const Byte>(msg.messageData)), std::move(callback), fromPeerId);
}

void EntityNetworkSession::onReceiveSystemMessageResponse(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageSystemMsgResponse& msg)
{
	auto iter = pendingSysMsgResponses.find(msg.msgId);
	if (iter == pendingSysMsgResponses.end()) {
		Logger::logWarning("Unexpected system message response received.");
		return;
	}

	iter->second.callback(nullptr, msg.responseData);
	pendingSysMsgResponses.erase(iter);
}

void EntityNetworkSession::onReceiveJoinWorld(NetworkSession::PeerId fromPeerId)
{
	Logger::logDev("Peer " + toString(int(fromPeerId)) + " joining world...");
	for (auto& peer: peers) {
		if (peer.getPeerId() == fromPeerId) {
			peer.onJoinedWorld();
			break;
		}
	}
}

void EntityNetworkSession::onReceiveGetLobbyInfo(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageGetLobbyInfo& msg)
{
	sendUpdatedLobbyInfos(fromPeerId);
}

void EntityNetworkSession::onReceiveUpdateLobbyInfo(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageUpdateLobbyInfo& msg)
{
	if (fromPeerId == 0) {
		if (listener) {
			listener->onReceiveLobbyInfo(ConfigNode(msg.lobbyInfo));
		}
	}
}

void EntityNetworkSession::onReceiveSetLobbyInfo(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageSetLobbyInfo& msg)
{
	if (listener) {
		if (listener->setLobbyInfo(fromPeerId, msg.lobbyInfo)) {
			sendUpdatedLobbyInfos({});
		}
	}
}

void EntityNetworkSession::setupDictionary()
{
	serializationDictionary.addEntry("components");
	serializationDictionary.addEntry("children");
	serializationDictionary.addEntry("Transform2D");
	serializationDictionary.addEntry("position");
}

void EntityNetworkSession::setupByteSerializationInterpolators()
{
	Expects(listener != nullptr);
	if (listener) {
		listener->setupByteInterpolators(byteDataInterpolatorSet);
	}
}

ConfigNode EntityNetworkSession::getLobbyInfo()
{
	return listener ? listener->getLobbyInfo() : ConfigNode();
}

void EntityNetworkSession::sendUpdatedLobbyInfos(std::optional<NetworkSession::PeerId> toPeerId)
{
	auto lobbyInfo = getLobbyInfo();
	for (auto& peer: peers) {
		if (!toPeerId || toPeerId == peer.getPeerId()) {
			peer.sendLobbyInfo(ConfigNode(lobbyInfo));
		}
	}
	if (!toPeerId || toPeerId == 0) {
		listener->onReceiveLobbyInfo(lobbyInfo);
	}
}

World& EntityNetworkSession::getWorld() const
{
	Expects(factory);
	return factory->getWorld();
}

Resources& EntityNetworkSession::getResources() const
{
    return resources;
}

EntityFactory& EntityNetworkSession::getFactory() const
{
	Expects(factory);
	return *factory;
}

const EntityFactory::SerializationOptions& EntityNetworkSession::getEntitySerializationOptions() const
{
	Expects(factory);
	return entitySerializationOptions;
}

const EntityDataDelta::Options& EntityNetworkSession::getEntityDeltaOptions() const
{
	return deltaOptions;
}

const SerializerOptions& EntityNetworkSession::getByteSerializationOptions() const
{
	return byteSerializationOptions;
}

SerializationDictionary& EntityNetworkSession::getSerializationDictionary()
{
	return serializationDictionary;
}

const IByteDataInterpolatorSet* EntityNetworkSession::getByteDataInterpolatorSet() const
{
	return &byteDataInterpolatorSet;
}

Time EntityNetworkSession::getMinSendInterval() const
{
	return 0.05;
}

void EntityNetworkSession::onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId)
{
	if (listener) {
		listener->onRemoteEntityCreated(entity, peerId);
	}
}

void EntityNetworkSession::requestSetupInterpolators(DataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote)
{
	interpolatorSet.markReady();

	if (!entity.isSerializable()) {
		return;
	}

	if (listener) {
		listener->setupInterpolators(interpolatorSet, entity, remote);

		for (const auto& c: entity.getChildren()) {
			if (!c.getOwnerPeerId()) {
				requestSetupInterpolators(interpolatorSet, c, remote);
			}
		}
	}
}

void EntityNetworkSession::requestSetupByteDataInterpolators(ByteDataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote)
{
	if (!entity.isSerializable()) {
		return;
	}

	if (listener) {
		listener->setupByteInterpolators(interpolatorSet, entity);

		for (const auto& c: entity.getChildren()) {
			if (!c.getOwnerPeerId()) {
				requestSetupByteDataInterpolators(interpolatorSet, c, remote);
			}
		}
	}
}

void EntityNetworkSession::setupOutboundInterpolators(EntityRef entity)
{
	if (!entity.isSerializable()) {
		return;
	}

	if (listener) {
        std::unique_lock lock(outboundInterpolatorLock);
		auto& interpolatorSet = entity.setupNetwork(session->getMyPeerId().value());
		if (!interpolatorSet.isReady()) {
			requestSetupInterpolators(interpolatorSet, entity, false);

			auto& byteDataInterpolatorSet = entity.getComponent<NetworkComponent>().byteDataInterpolatorSet;
			requestSetupByteDataInterpolators(byteDataInterpolatorSet, entity, false);
		}
	}
}

void EntityNetworkSession::startGame()
{
	if (isHost()) {
		gameStarted = true;
		auto& sharedData = session->getMutableSessionSharedData<EntitySessionSharedData>();
		sharedData.gameStarted = true;
		sharedData.markModified();
	}
}

void EntityNetworkSession::joinGame()
{
	if (!isHost() && !peers.empty()) {
		peers.back().requestJoinWorld();
	}
}

bool EntityNetworkSession::isGameStarted() const
{
	if (isHost()) {
		return gameStarted;
	} else {
		return session->hasSessionSharedData() && session->getSessionSharedData<EntitySessionSharedData>().gameStarted;
	}
}

bool EntityNetworkSession::isReadyToStartGame() const
{
	return readyToStartGame;
}

bool EntityNetworkSession::isLobbyReady() const
{
	return lobbyReady;
}

bool EntityNetworkSession::isEntityInView(EntityRef entity, const EntityClientSharedData& clientData, NetworkSession::PeerId peerId) const
{
	Expects(listener);
	return listener->isEntityInView(entity, clientData, peerId);
}

Vector<Rect4i> EntityNetworkSession::getRemoteViewPorts() const
{
	Vector<Rect4i> result;
	for (auto& peer: peers) {
		const auto* data = session->tryGetClientSharedData<EntityClientSharedData>(peer.getPeerId());
		if (data && data->viewRect) {
			result.push_back(data->viewRect.value());
		}
	}
	return result;
}

bool EntityNetworkSession::isHost() const
{
	return session->getType() == NetworkSessionType::Host;
}

bool EntityNetworkSession::isRemote(ConstEntityRef entity) const
{
    auto entityOwner = entity.getAuthorityPeerId();

    if (!entityOwner) {
        entityOwner = entity.getOwnerPeerId();
    }

	if (!entityOwner) {
		return false;
	}
	
	return entityOwner != session->getMyPeerId();
}

NetworkSession& EntityNetworkSession::getSession() const
{
	Expects(session);
	return *session;
}

bool EntityNetworkSession::hasWorld() const
{
	return !!factory;
}

void EntityNetworkSession::onStartSession(NetworkSession::PeerId myPeerId)
{
	Logger::logDev("Starting session, I'm peer id " + toString(static_cast<int>(myPeerId)));
	if (myPeerId == 0) {
		lobbyReady = true;
	}
	listener->onStartSession(myPeerId);
	setupByteSerializationInterpolators();
}

void EntityNetworkSession::onPeerConnected(NetworkSession::PeerId peerId)
{
	Logger::logDev("Peer " + toString(static_cast<int>(peerId)) + " connected to EntityNetworkSession.");
	peers.emplace_back(*this, peerId);
}

void EntityNetworkSession::onPeerDisconnected(NetworkSession::PeerId peerId)
{
	for (auto& peer: peers) {
		if (peer.getPeerId() == peerId) {
			peer.destroy();
		}
	}
	std_ex::erase_if(peers, [](const EntityNetworkRemotePeer& p) { return !p.isAlive(); });

	if (listener) {
		listener->setLobbyInfo(peerId, {});
	}

	sendUpdatedLobbyInfos({});

	Logger::logDev("Peer " + toString(static_cast<int>(peerId)) + " disconnected from EntityNetworkSession.");
}

void EntityNetworkSession::findEntity(EntityNetworkId networkId, bool inbound, std::function<void(EntityId, NetworkSession::PeerId)> callback) const
{
	for (auto& peer: peers) {
		auto entityId = inbound ? peer.findInboundEntity(networkId) : peer.findOutboundEntity(networkId);
		if (entityId.isValid()) {
			callback(entityId, peer.getPeerId());
		}
	}
}

void EntityNetworkSession::logUpdates()
{
	for (auto& peer: peers) {
		peer.logUpdates();
	}
}

bool EntityNetworkSession::prepareChangeEntityAuthority(EntityId entityId, const NetworkComponent& networkComponent, std::optional<NetworkSession::PeerId> authorityId)
{
	const auto myPeerId = session->getMyPeerId().value(); // non-optional
	const auto ownerId = networkComponent.ownerId; // can be none

	// This looks complicated, but it's just trying to unravel who gives, takes or returns
	// authority to what peer, and notifies the right EntityNetworkRemotePeer instance about it.
	//
	// This whole block could be shortened down a lot, but this wouldn't be very readable.

	if (authorityId.has_value()) {
		// Authority is taken by someone.
		Expects(!networkComponent.authorityId.has_value());

		if (ownerId == myPeerId) {
			// Local peer is giving away authority.
			for (auto& peer: peers) {
				if (peer.getPeerId() == authorityId) {
					peer.prepareChangeEntityAuthority(entityId, myPeerId, ownerId.value(), authorityId);
					break;
				}
			}
			networkComponent.byteDataInterpolatorSet.reset();
		} else if (authorityId == myPeerId) {
			// Local peer is taking authority.
			for (auto& peer: peers) {
				if (peer.getPeerId() == ownerId) {
					peer.prepareChangeEntityAuthority(entityId, myPeerId, ownerId.value(), authorityId);
					break;
				}
			}
			networkComponent.byteDataInterpolatorSet.reset();
		} else {
			// Remote peer is taking authority from another remote peer. We are not involved.
		}
	} else {
		// Authority is given back to owner.
		if (networkComponent.authorityId == myPeerId) {
			// Local peer is losing authority. Revoke the outbound entity.
			for (auto& peer: peers) {
				if (peer.getPeerId() == ownerId) {
					peer.prepareChangeEntityAuthority(entityId, myPeerId, ownerId.value(), authorityId);
					break;
				}
			}
			networkComponent.byteDataInterpolatorSet.reset();
		} else if (ownerId == myPeerId) {
			// Authority returned to local peer. Revoke the inbound entity.
			for (auto& peer: peers) {
				if (peer.getPeerId() == networkComponent.authorityId) {
					peer.prepareChangeEntityAuthority(entityId, myPeerId, ownerId.value(), authorityId);
					break;
				}
			}
			networkComponent.byteDataInterpolatorSet.reset();
		} else {
			// Remote peer is returning authority to another remote peer. We are not involved.
		}
	}

	return true;
}

bool EntityNetworkSession::allowComponentAddedForFastUpdate(uint16_t componentId) const
{
	if (listener) {
		return listener->allowComponentAddedForFastUpdate(componentId);
	}
	return false;
}

void EntitySessionSharedData::serialize(Serializer& s) const
{
	s << gameStarted;
}

void EntitySessionSharedData::deserialize(Deserializer& s)
{
	s >> gameStarted;
}

void EntityClientSharedData::serialize(Serializer& s) const
{
	s << viewRect;
}

void EntityClientSharedData::deserialize(Deserializer& s)
{
	s >> viewRect;
}
