#include "entity/entity_network_remote_peer.h"

#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

EntityNetworkRemotePeer::EntityNetworkRemotePeer(NetworkSession::PeerId peerId)
	: peerId(peerId)
{}

NetworkSession::PeerId EntityNetworkRemotePeer::getPeerId() const
{
	return peerId;
}

void EntityNetworkRemotePeer::sendEntities(Time t, EntityFactory& entityFactory, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
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

void EntityNetworkRemotePeer::receiveEntities(EntityFactory& entityFactory)
{
	// TODO
}

void EntityNetworkRemotePeer::destroy(World& world)
{
	if (alive) {
		for (const auto& [k, v]: inboundEntities) {
			world.destroyEntity(v.worldId);
		}
		alive = false;
	}
}

bool EntityNetworkRemotePeer::isAlive() const
{
	return alive;
}

void EntityNetworkRemotePeer::createEntity(EntityFactory& entityFactory, EntityRef entity)
{
	OutboundEntity result;

	result.alive = true;
	result.networkId = assignId();
	result.data = {}; // TODO

	//OutboundNetworkPacket packet;
	//session->sendToPeer(packet, peerId);
	
	outboundEntities[entity.getEntityId()] = std::move(result);
}

void EntityNetworkRemotePeer::destroyEntity(EntityFactory& entityFactory, OutboundEntity& remote)
{
	// TODO: send to remote
}

void EntityNetworkRemotePeer::updateEntity(EntityFactory& entityFactory, OutboundEntity& remote, EntityRef entity)
{
	// TODO: send to remote
}

uint16_t EntityNetworkRemotePeer::assignId()
{
	// TODO
	return 0;
}
