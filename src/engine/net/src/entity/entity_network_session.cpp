#include "entity/entity_network_session.h"

#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

EntityNetworkSession::EntityNetworkSession(std::shared_ptr<NetworkSession> session)
	: session(std::move(session))
{
	Expects(this->session);
	this->session->addListener(this);
}

EntityNetworkSession::~EntityNetworkSession()
{
	session->removeListener(this);
}

void EntityNetworkSession::init(World& world, Resources& resources, std::set<String> ignoreComponents, IEntityNetworkSessionListener* listener)
{
	factory = std::make_shared<EntityFactory>(world, resources);
	serializationOptions.type = EntitySerialization::Type::SaveData;
	this->listener = listener;

	deltaOptions.preserveOrder = false;
	deltaOptions.shallow = false;
	deltaOptions.ignoreComponents = std::move(ignoreComponents);
}

void EntityNetworkSession::sendLocalEntities(Time t, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
{
	for (auto& peer: peers) {
		peer.sendEntities(t, entityIds);
	}
}

void EntityNetworkSession::receiveRemoteEntities()
{
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
		}
	}
}

void EntityNetworkSession::onReceiveEntityUpdate(NetworkSession::PeerId fromPeerId, EntityNetworkHeaderType type, InboundNetworkPacket packet)
{
	Logger::logDev("Received entity update from " + toString(static_cast<int>(fromPeerId)) + " of type " + toString(static_cast<int>(type)));
	
	for (auto& peer: peers) {
		if (peer.getPeerId() == fromPeerId) {
			peer.receiveEntityPacket(fromPeerId, type, std::move(packet));
			return;
		}
	}
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

const EntityFactory::SerializationOptions& EntityNetworkSession::getSerializationOptions() const
{
	Expects(factory);
	return serializationOptions;
}

const EntityDataDelta::Options& EntityNetworkSession::getEntityDeltaOptions() const
{
	return deltaOptions;
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
}

void EntityNetworkSession::onPeerConnected(NetworkSession::PeerId peerId)
{
	peers.push_back(EntityNetworkRemotePeer(*this, peerId));

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

