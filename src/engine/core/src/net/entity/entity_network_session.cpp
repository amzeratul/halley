#include "halley/net/entity/entity_network_session.h"

#include <cassert>

#include "halley/bytes/compression.h"
#include "halley/entity/data_interpolator.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/system.h"
#include "halley/entity/world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"

class NetworkComponent;
using namespace Halley;

EntityNetworkSession::EntityNetworkSession(std::shared_ptr<NetworkSession> session, Resources& resources, std::set<String> ignoreComponents, IEntityNetworkSessionListener* listener)
	: resources(resources)
	, listener(listener)
	, session(std::move(session))
{
	Expects(this->session);
	Expects(this->listener);
	
	this->session->addListener(this);
	this->session->setSharedDataHandler(this);

	entitySerializationOptions.type = EntitySerialization::Type::Network;

	deltaOptions.preserveOrder = false;
	deltaOptions.shallow = false;
	deltaOptions.deltaComponents = true;
	deltaOptions.allowNonSerializable = false;
	deltaOptions.ignoreComponents = std::move(ignoreComponents);
	deltaOptions.resources = &resources;

	setupDictionary();
	byteSerializationOptions.version = SerializerOptions::maxVersion;
	byteSerializationOptions.dictionary = &serializationDictionary;
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

void EntityNetworkSession::sendUpdates(Time t, Rect4i viewRect, gsl::span<const EntityNetworkUpdateInfo> entityIds)
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
	for (auto& peer: peers) {
		peer.sendEntities(t, entityIds, session->getClientSharedData<EntityClientSharedData>(peer.getPeerId()));
	}

	sendMessages();
	session->update(t);
}

void EntityNetworkSession::sendToAll(EntityNetworkMessage msg)
{
	outbox[-1].push_back(std::move(msg));
}

void EntityNetworkSession::sendToPeer(EntityNetworkMessage msg, NetworkSession::PeerId peerId)
{
	outbox[peerId].push_back(std::move(msg));
}

void EntityNetworkSession::sendMessages()
{
	auto tryCompress = [&](size_t startIdx, size_t count, const Vector<EntityNetworkMessage>& msgs) -> std::optional<Bytes>
	{
		auto data = Serializer::toBytes(msgs.span().subspan(startIdx, count), byteSerializationOptions);
		auto compressed = Compression::lz4Compress(gsl::as_bytes(gsl::span<const Byte>(data)));
		if (compressed.size() <= 2000) {
			return std::move(compressed);
		} else {
			return std::nullopt;
		}
	};

	for (const auto& [peerId, msgs]: outbox) {
		size_t startIdx = 0;
		size_t curCount = msgs.size();

		while (startIdx < msgs.size()) {
			if (auto data = tryCompress(startIdx, curCount, msgs)) {
				auto packet = OutboundNetworkPacket(*data);
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
					// It might be able to fit more, but halving will approach the solution faster than trying to find the exact number, at a cost of a bit of inefficiency
					curCount /= 2;
				} else {
					Logger::logError("Individual entity network message is too big to send over network, skipping it!");
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

		Bytes bytes;
		bytes.resize(32 * 1024);
		const auto size = Compression::lz4Decompress(packet.getBytes(), gsl::as_writable_bytes(bytes.span()));
		if (size) {
			bytes.resize(*size);
		} else {
			Logger::logError("Failed to decompress network packet");
			continue;
		}
		auto msgs = Deserializer::fromBytes<Vector<EntityNetworkMessage>>(bytes, byteSerializationOptions);

		for (auto& msg: msgs) {
			if (canProcessMessage(msg)) {
				processMessage(fromPeerId, std::move(msg));
			} else {
				queuedPackets.emplace_back(QueuedMessage{ fromPeerId, std::move(msg) });
			}
		}
	}
}

bool EntityNetworkSession::canProcessMessage(const EntityNetworkMessage& msg) const
{
	return factory || !msg.needsInitialization();
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
		readyToStart = true;
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
		Logger::logError("Received message for entity " + toString(msg.entityUUID) + ", but entity was not found.");
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


void EntityNetworkSession::setupDictionary()
{
	serializationDictionary.addEntry("components");
	serializationDictionary.addEntry("children");
	serializationDictionary.addEntry("Transform2D");
	serializationDictionary.addEntry("position");
}

World& EntityNetworkSession::getWorld() const
{
	Expects(factory);
	return factory->getWorld();
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

void EntityNetworkSession::setupOutboundInterpolators(EntityRef entity)
{
	if (!entity.isSerializable()) {
		return;
	}

	if (listener) {
		auto& interpolatorSet = entity.setupNetwork(session->getMyPeerId().value());
		if (!interpolatorSet.isReady()) {
			requestSetupInterpolators(interpolatorSet, entity, false);
		}
	}
}

bool EntityNetworkSession::isReadyToStart() const
{
	return readyToStart;
}

bool EntityNetworkSession::isEntityInView(EntityRef entity, const EntityClientSharedData& clientData) const
{
	Expects(listener);
	return listener->isEntityInView(entity, clientData);
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

bool EntityNetworkSession::isHost()
{
	return session->getType() == NetworkSessionType::Host;
}

bool EntityNetworkSession::isRemote(ConstEntityRef entity) const
{
	const auto entityOwner = entity.getOwnerPeerId();
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

Resources& EntityNetworkSession::getResources() const
{
	return resources;
}

void EntityNetworkSession::onStartSession(NetworkSession::PeerId myPeerId)
{
	Logger::logDev("Starting session, I'm peer id " + toString(static_cast<int>(myPeerId)));
	if (myPeerId == 0) {
		readyToStart = true;
	}
	listener->onStartSession(myPeerId);
}

void EntityNetworkSession::onPeerConnected(NetworkSession::PeerId peerId)
{
	peers.emplace_back(*this, peerId);

	Logger::logDev("Peer " + toString(static_cast<int>(peerId)) + " connected to EntityNetworkSession.");
}

void EntityNetworkSession::onPeerDisconnected(NetworkSession::PeerId peerId)
{
	for (auto& peer: peers) {
		if (peer.getPeerId() == peerId) {
			peer.destroy();
		}
	}
	std_ex::erase_if(peers, [](const EntityNetworkRemotePeer& p) { return !p.isAlive(); });

	Logger::logDev("Peer " + toString(static_cast<int>(peerId)) + " disconnected from EntityNetworkSession.");
}

std::unique_ptr<SharedData> EntityNetworkSession::makeSessionSharedData()
{
	return std::make_unique<EntitySessionSharedData>();
}

std::unique_ptr<SharedData> EntityNetworkSession::makePeerSharedData()
{
	return std::make_unique<EntityClientSharedData>();
}

void EntityClientSharedData::serialize(Serializer& s) const
{
	s << viewRect;
}

void EntityClientSharedData::deserialize(Deserializer& s)
{
	s >> viewRect;
}
