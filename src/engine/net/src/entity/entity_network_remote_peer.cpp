#include "entity/entity_network_remote_peer.h"

#include "entity/entity_network_session.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

EntityNetworkRemotePeer::EntityNetworkRemotePeer(EntityNetworkSession& parent, NetworkSession::PeerId peerId)
	: parent(&parent)
	, peerId(peerId)
{}

NetworkSession::PeerId EntityNetworkRemotePeer::getPeerId() const
{
	return peerId;
}

void EntityNetworkRemotePeer::sendEntities(Time t, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
{
	if (!isAlive()) {
		return;
	}
	
	// Mark all as not alive
	for (auto& e: outboundEntities) {
		e.second.alive = false;
	}
	
	for (auto [entityId, ownerId]: entityIds) {
		if (ownerId == peerId) {
			// Don't send updates back to the owner
			continue;
		}

		auto entity = parent->getWorld().getEntity(entityId);

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

void EntityNetworkRemotePeer::receiveEntities()
{
	if (!isAlive()) {
		return;
	}
	
	// TODO
}

void EntityNetworkRemotePeer::destroy(World& world)
{
	if (alive) {
		for (const auto& [k, v]: inboundEntities) {
			world.destroyEntity(v.worldId);
		}
		inboundEntities.clear();
		alive = false;
	}
}

bool EntityNetworkRemotePeer::isAlive() const
{
	return alive;
}

void EntityNetworkRemotePeer::createEntity(EntityRef entity)
{
	OutboundEntity result;

	result.alive = true;
	result.networkId = assignId();
	result.data = parent->getFactory().serializeEntity(entity, parent->getSerializationOptions());

	auto deltaData = parent->getFactory().entityDataToPrefabDelta(result.data, entity.getPrefab(), parent->getEntityDeltaOptions());
	
	EntityHeader header;
	header.type = EntityHeaderType::Create;
	header.entityId = result.networkId;
	
	const auto bytes = Serializer::toBytes(result.data);
	auto packet = OutboundNetworkPacket(bytes);
	packet.addHeader(header);
	parent->getSession().sendToPeer(packet, peerId);
	
	outboundEntities[entity.getEntityId()] = std::move(result);
}

void EntityNetworkRemotePeer::updateEntity(OutboundEntity& remote, EntityRef entity)
{
	auto newData = parent->getFactory().serializeEntity(entity, parent->getSerializationOptions());
	
}

void EntityNetworkRemotePeer::destroyEntity(OutboundEntity& remote)
{
	// TODO: send to remote
}

uint16_t EntityNetworkRemotePeer::assignId()
{
	// TODO
	return 0;
}
