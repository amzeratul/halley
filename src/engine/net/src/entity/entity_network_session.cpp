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

void EntityNetworkSession::init(World& world, Resources& resources)
{
	factory = std::make_shared<EntityFactory>(world, resources);
}

void EntityNetworkSession::sendLocalEntities(Time t, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
{
	for (auto& peer: peers) {
		peer.sendEntities(t, *factory, entityIds);
	}
}

void EntityNetworkSession::receiveRemoteEntities()
{
	for (auto& peer: peers) {
		peer.receiveEntities(*factory);
	}
}

void EntityNetworkSession::onStartSession(NetworkSession::PeerId myPeerId)
{
	// TODO
}

void EntityNetworkSession::onPeerConnected(NetworkSession::PeerId peerId)
{
	peers.push_back(RemotePeer(peerId));
	Logger::logDev("Peer " + toString(static_cast<int>(peerId)) + " connected to EntityNetworkSession.");
}

void EntityNetworkSession::onPeerDisconnected(NetworkSession::PeerId peerId)
{
	Logger::logDev("Peer " + toString(static_cast<int>(peerId)) + " disconnected from EntityNetworkSession.");
	for (auto& peer: peers) {
		if (peer.getPeerId() == peerId) {
			peer.destroy(factory->getWorld());
		}
	}
	std_ex::erase_if(peers, [](const RemotePeer& p) { return !p.isAlive(); });
}



EntityNetworkSession::RemotePeer::RemotePeer(NetworkSession::PeerId peerId)
	: peerId(peerId)
{}

NetworkSession::PeerId EntityNetworkSession::RemotePeer::getPeerId() const
{
	return peerId;
}

void EntityNetworkSession::RemotePeer::sendEntities(Time t, EntityFactory& entityFactory, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
{
	// Mark all as not alive
	for (auto& e: outboundEntities) {
		e.second.alive = false;
	}
	
	for (auto [entityId, ownerId]: entityIds) {
		if (ownerId == peerId) {
			// Don't send updates back to the owner
			continue;
		}

		auto entity = entityFactory.getWorld().getEntity(entityId);

		const auto iter = outboundEntities.find(entityId);
		if (iter == outboundEntities.end()) {
			createEntity(entityFactory, entity);
		} else {
			updateEntity(entityFactory, iter->second, entity);
		}
	}

	// Destroy dead entities
	for (auto& e: outboundEntities) {
		if (!e.second.alive) {
			destroyEntity(entityFactory, e.second);
		}
	}
	std_ex::erase_if_value(outboundEntities, [](const OutboundEntity& e) { return !e.alive; });
}

void EntityNetworkSession::RemotePeer::receiveEntities(EntityFactory& entityFactory)
{
	// TODO
}

void EntityNetworkSession::RemotePeer::destroy(World& world)
{
	if (alive) {
		for (const auto& [k, v]: inboundEntities) {
			world.destroyEntity(v.worldId);
		}
		alive = false;
	}
}

bool EntityNetworkSession::RemotePeer::isAlive() const
{
	return alive;
}

void EntityNetworkSession::RemotePeer::createEntity(EntityFactory& entityFactory, EntityRef entity)
{
	OutboundEntity result;

	result.alive = true;
	result.networkId = assignId();
	result.data = {}; // TODO

	//OutboundNetworkPacket packet;
	//session->sendToPeer(packet, peerId);
	
	outboundEntities[entity.getEntityId()] = std::move(result);
}

void EntityNetworkSession::RemotePeer::destroyEntity(EntityFactory& entityFactory, OutboundEntity& remote)
{
	// TODO: send to remote
}

void EntityNetworkSession::RemotePeer::updateEntity(EntityFactory& entityFactory, OutboundEntity& remote, EntityRef entity)
{
	// TODO: send to remote
}

uint16_t EntityNetworkSession::RemotePeer::assignId()
{
	// TODO
	return 0;
}
