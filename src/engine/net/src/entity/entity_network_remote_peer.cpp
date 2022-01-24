#include "entity/entity_network_remote_peer.h"

#include "entity/entity_network_session.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"

class NetworkComponent;
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
	Expects(isAlive());
	
	// Mark all as not alive
	for (auto& e: outboundEntities) {
		e.second.alive = false;
	}
	
	for (auto [entityId, ownerId]: entityIds) {
		if (ownerId == peerId) {
			// Don't send updates back to the owner
			continue;
		}

		const auto entity = parent->getWorld().getEntity(entityId);

		if (const auto iter = outboundEntities.find(entityId); iter == outboundEntities.end()) {
			sendCreateEntity(entity);
		} else {
			sendUpdateEntity(t, iter->second, entity);
		}
	}

	// Destroy dead entities
	for (auto& e: outboundEntities) {
		if (!e.second.alive) {
			sendDestroyEntity(e.second);
		}
	}
	std_ex::erase_if_value(outboundEntities, [](const OutboundEntity& e) { return !e.alive; });
}

void EntityNetworkRemotePeer::receiveEntityPacket(NetworkSession::PeerId fromPeerId, EntityNetworkHeaderType type, InboundNetworkPacket packet)
{
	Expects(isAlive());

	EntityNetworkEntityHeader header;
	packet.extractHeader(header);
	const auto networkEntityId = header.entityId;

	if (type == EntityNetworkHeaderType::Create) {
		receiveCreateEntity(networkEntityId, packet.getBytes());
	} else if (type == EntityNetworkHeaderType::Update) {
		receiveUpdateEntity(networkEntityId, packet.getBytes());
	} else if (type == EntityNetworkHeaderType::Destroy) {
		receiveDestroyEntity(networkEntityId);
	}
}

void EntityNetworkRemotePeer::destroy()
{
	if (alive) {
		if (parent->hasWorld()) {
			auto& world = parent->getWorld();
			for (const auto& [k, v] : inboundEntities) {
				world.destroyEntity(v.worldId);
			}
		}
		
		inboundEntities.clear();
		alive = false;
	}
}

bool EntityNetworkRemotePeer::isAlive() const
{
	return alive;
}

uint16_t EntityNetworkRemotePeer::assignId()
{
	for (uint16_t i = 0; i < std::numeric_limits<uint16_t>::max() - 1; ++i) {
		const uint16_t id = i + nextId;
		if (!allocatedOutboundIds.contains(id)) {
			allocatedOutboundIds.insert(id);
			nextId = id + 1;
			return id;
		}
	}
	throw Exception("Unable to allocate network id for entity.", HalleyExceptions::Network);
}

void EntityNetworkRemotePeer::sendCreateEntity(EntityRef entity)
{
	OutboundEntity result;

	result.networkId = assignId();
	result.data = parent->getFactory().serializeEntity(entity, parent->getEntitySerializationOptions());

	auto deltaData = parent->getFactory().entityDataToPrefabDelta(result.data, entity.getPrefab(), parent->getEntityDeltaOptions());
	const auto bytes = Serializer::toBytes(result.data, parent->getByteSerializationOptions());
	
	auto packet = OutboundNetworkPacket(bytes);
	packet.addHeader(EntityNetworkEntityHeader(result.networkId));
	packet.addHeader(EntityNetworkHeader(EntityNetworkHeaderType::Create));
	parent->getSession().sendToPeer(packet, peerId);
	
	outboundEntities[entity.getEntityId()] = std::move(result);

	Logger::logDev("Sending create " + entity.getName() + ": " + toString(bytes.size()) + " bytes to peer " + toString(static_cast<int>(peerId)));
	Logger::logDev("Create:\n" + EntityData(deltaData).toYAML() + "\n");
}

void EntityNetworkRemotePeer::sendUpdateEntity(Time t, OutboundEntity& remote, EntityRef entity)
{
	remote.alive = true; // Important: mark it back alive
	remote.timeSinceSend += t;
	if (remote.timeSinceSend < parent->getMinSendInterval()) {
		return;
	}
	
	auto newData = parent->getFactory().serializeEntity(entity, parent->getEntitySerializationOptions());
	auto deltaData = EntityDataDelta(remote.data, newData, parent->getEntityDeltaOptions());

	if (deltaData.hasChange()) {
		parent->onPreSendDelta(deltaData);
	}

	if (deltaData.hasChange()) {
		remote.data = std::move(newData);
		remote.timeSinceSend = 0;

		const auto bytes = Serializer::toBytes(deltaData, parent->getByteSerializationOptions());
		
		auto packet = OutboundNetworkPacket(bytes);
		packet.addHeader(EntityNetworkEntityHeader(remote.networkId));
		packet.addHeader(EntityNetworkHeader(EntityNetworkHeaderType::Update));
		parent->getSession().sendToPeer(packet, peerId);

		auto str = std::string(bytes.size(), '.');
		for (size_t i = 0; i < bytes.size(); ++i) {
			if (bytes[i] >= 0x20) {
				str[i] = bytes[i];
			}
		}

		Logger::logDev("Sending update " + entity.getName() + ": " + toString(bytes.size()) + " bytes to peer " + toString(static_cast<int>(peerId)));
		Logger::logDev("Update:\n" + EntityData(deltaData).toYAML() + "\n");
	}
}

void EntityNetworkRemotePeer::sendDestroyEntity(OutboundEntity& remote)
{
	auto packet = OutboundNetworkPacket(Bytes());
	packet.addHeader(EntityNetworkEntityHeader(remote.networkId));
	packet.addHeader(EntityNetworkHeader(EntityNetworkHeaderType::Destroy));
	parent->getSession().sendToPeer(packet, peerId);
	allocatedOutboundIds.erase(remote.networkId);

	//Logger::logDev("Sending destroy entity to peer " + toString(static_cast<int>(peerId)));
}

void EntityNetworkRemotePeer::receiveCreateEntity(EntityNetworkId id, gsl::span<const gsl::byte> data)
{
	const auto iter = inboundEntities.find(id);
	if (iter != inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(id)) + " already exists from peer " + toString(static_cast<int>(peerId)));
		return;
	}

	auto delta = Deserializer::fromBytes<EntityDataDelta>(data, parent->getByteSerializationOptions());
	auto [entityData, prefab, prefabUUID] = parent->getFactory().prefabDeltaToEntityData(delta);

	auto entity = parent->getFactory().createEntity(entityData);
	if (prefab) {
		entity.setPrefab(prefab, prefabUUID);
	}

	entity.setupNetwork(peerId);
	
	InboundEntity remote;
	remote.data = std::move(entityData);
	remote.worldId = entity.getEntityId();

	inboundEntities[id] = std::move(remote);

	parent->onRemoteEntityCreated(entity, peerId);
}

void EntityNetworkRemotePeer::receiveUpdateEntity(EntityNetworkId id, gsl::span<const gsl::byte> data)
{
	const auto iter = inboundEntities.find(id);
	if (iter == inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(id)) + " not found from peer " + toString(static_cast<int>(peerId)));
		return;
	}
	auto& remote = iter->second;

	auto entity = parent->getWorld().getEntity(remote.worldId);
	if (!entity.isValid()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(id)) + " not alive in the world from peer " + toString(static_cast<int>(peerId)));
		return;
	}
	
	auto delta = Deserializer::fromBytes<EntityDataDelta>(data, parent->getByteSerializationOptions());

	parent->getFactory().updateEntity(entity, delta, static_cast<int>(EntitySerialization::Type::SaveData));
	remote.data.applyDelta(delta);
}

void EntityNetworkRemotePeer::receiveDestroyEntity(EntityNetworkId id)
{
	const auto iter = inboundEntities.find(id);
	if (iter == inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(id)) + " not found from peer " + toString(static_cast<int>(peerId)));
		return;
	}
	auto& remote = iter->second;

	parent->getWorld().destroyEntity(remote.worldId);

	inboundEntities.erase(id);
}
