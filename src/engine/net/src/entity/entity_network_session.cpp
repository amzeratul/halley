#include "entity/entity_network_session.h"

#include "halley/entity/world.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

EntityNetworkSession::EntityNetworkSession(std::shared_ptr<NetworkSession> session)
	: session(std::move(session))
{
}

void EntityNetworkSession::sendLocalEntities(Time t, World& world, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
{
	for (auto& peer: peers) {
		peer.updateEntities(t, world, entityIds);
	}
}

void EntityNetworkSession::receiveRemoteEntities(World& world, Resources& resources)
{
	// TODO
}

EntityNetworkSession::RemotePeer::RemotePeer(NetworkSession::PeerId peerId)
	: peerId(peerId)
{}

NetworkSession::PeerId EntityNetworkSession::RemotePeer::getPeerId() const
{
	return peerId;
}

void EntityNetworkSession::RemotePeer::updateEntities(Time t, World& world, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
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

		auto entity = world.getEntity(entityId);

		const auto iter = outboundEntities.find(entityId);
		if (iter == outboundEntities.end()) {
			createEntity(entity);
		} else {
			updateEntity(iter->second, entity);
		}
	}

	// Destroy dead entities
	for (auto& e: outboundEntities) {
		if (!e.second.alive) {
			destroyEntity(e.second);
		}
	}
	std_ex::erase_if_value(outboundEntities, [](const OutboundEntity& e) { return !e.alive; });
}

void EntityNetworkSession::RemotePeer::createEntity(EntityRef entity)
{
	OutboundEntity result;

	result.alive = true;
	result.networkId = assignId();
	result.data = {}; // TODO

	// TODO: send to remote
	
	outboundEntities[entity.getEntityId()] = std::move(result);
}

void EntityNetworkSession::RemotePeer::destroyEntity(OutboundEntity& remote)
{
	// TODO: send to remote
}

void EntityNetworkSession::RemotePeer::updateEntity(OutboundEntity& remote, EntityRef entity)
{
	// TODO: send to remote
}

uint16_t EntityNetworkSession::RemotePeer::assignId()
{
	// TODO
	return 0;
}
