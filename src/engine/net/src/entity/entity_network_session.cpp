#include "entity/entity_network_session.h"

#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"

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

	entitySerializationOptions.type = EntitySerialization::Type::SaveData;

	deltaOptions.preserveOrder = false;
	deltaOptions.shallow = false;
	deltaOptions.ignoreComponents = std::move(ignoreComponents);

	setupDictionary();
	byteSerializationOptions.version = SerializerOptions::maxVersion;
	byteSerializationOptions.dictionary = &serializationDictionary;
}

EntityNetworkSession::~EntityNetworkSession()
{
	session->removeListener(this);
}

void EntityNetworkSession::setWorld(World& world)
{
	factory = std::make_shared<EntityFactory>(world, resources);

	// Clear queue
	if (!queuedPackets.empty()) {
		for (auto& qp: queuedPackets) {
			onReceiveEntityUpdate(qp.fromPeerId, qp.type, std::move(qp.packet));
		}
		queuedPackets.clear();
	}
}

void EntityNetworkSession::sendUpdates(Time t, Rect4i viewRect, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
{
	// Update viewport
	auto& data = session->getMySharedData<EntityClientSharedData>();
	const bool first = !data.viewRect;
	data.viewRect = viewRect;
	if (first || data.getTimeSinceLastSend() > 0.05) {
		data.markModified();
	}

	// Update entities
	for (auto& peer: peers) {
		peer.sendEntities(t, entityIds, session->getClientSharedData<EntityClientSharedData>(peer.getPeerId()));
	}

	session->update(t);
}

void EntityNetworkSession::receiveUpdates()
{
	session->update(0.0);

	while (auto result = session->receive()) {
		const auto fromPeerId = result->first;
		auto& packet = result->second;
		
		EntityNetworkHeader header;
		packet.extractHeader(header);

		switch (header.type) {
		case EntityNetworkHeaderType::Create:
		case EntityNetworkHeaderType::Destroy:
		case EntityNetworkHeaderType::Update:
			onReceiveEntityUpdate(fromPeerId, header.type, std::move(packet));
			break;
		case EntityNetworkHeaderType::ReadyToStart:
			onReceiveReady(fromPeerId);
			break;
		}
	}
}

void EntityNetworkSession::onReceiveEntityUpdate(NetworkSession::PeerId fromPeerId, EntityNetworkHeaderType type, InboundNetworkPacket packet)
{
	if (factory) {
		for (auto& peer: peers) {
			if (peer.getPeerId() == fromPeerId) {
				peer.receiveEntityPacket(fromPeerId, type, std::move(packet));
				return;
			}
		}
	} else {
		// Factory not ready, queue them up for later
		queuedPackets.emplace_back(QueuedPacket{ fromPeerId, type, std::move(packet) });
	}
}

void EntityNetworkSession::onReceiveReady(NetworkSession::PeerId fromPeerId)
{
	Logger::logDev("onReceiveReady from " + toString(int(fromPeerId)));
	if (fromPeerId == 0) {
		readyToStart = true;
	}
}

void EntityNetworkSession::setupDictionary()
{
	serializationDictionary.addEntry("components");
	serializationDictionary.addEntry("children");
	serializationDictionary.addEntry("Transform2D");
	serializationDictionary.addEntry("position");

	// HACK
	serializationDictionary.addEntry("Velocity");
	serializationDictionary.addEntry("Character");
	serializationDictionary.addEntry("velocity");
	serializationDictionary.addEntry("facing");
	serializationDictionary.addEntry("moveInput");
	serializationDictionary.addEntry("Calendar");
	serializationDictionary.addEntry("time");
	serializationDictionary.addEntry("fract");
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

void EntityNetworkSession::onPreSendDelta(EntityDataDelta& delta)
{
	if (listener) {
		listener->onPreSendDelta(delta);
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
		readyToStart = true;
	}
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
