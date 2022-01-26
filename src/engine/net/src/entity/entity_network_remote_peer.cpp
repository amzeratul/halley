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

void EntityNetworkRemotePeer::sendEntities(Time t, gsl::span<const std::pair<EntityId, uint8_t>> entityIds, const EntityClientSharedData& clientData)
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
		if (peerId == 0 || parent->isEntityInView(entity, clientData)) { // Always send to host
			if (const auto iter = outboundEntities.find(entityId); iter == outboundEntities.end()) {
				sendCreateEntity(entity);
			} else {
				sendUpdateEntity(t, iter->second, entity);
			}
		}
	}

	// Destroy dead entities
	for (auto& e: outboundEntities) {
		if (!e.second.alive) {
			sendDestroyEntity(e.second);
		}
	}
	std_ex::erase_if_value(outboundEntities, [](const OutboundEntity& e) { return !e.alive; });

	if (!hasSentData) {
		hasSentData = true;
		onFirstDataBatchSent();
	}
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
	auto bytes = Serializer::toBytes(result.data, parent->getByteSerializationOptions());
	const auto size = send(EntityNetworkHeaderType::Create, result.networkId, std::move(bytes));
	
	outboundEntities[entity.getEntityId()] = std::move(result);

	Logger::logDev("Sending create " + entity.getName() + ": " + toString(size) + " bytes to peer " + toString(static_cast<int>(peerId)));
	//Logger::logDev("Create:\n" + EntityData(deltaData).toYAML() + "\n");
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

		auto bytes = Serializer::toBytes(deltaData, parent->getByteSerializationOptions());
		const auto size = send(EntityNetworkHeaderType::Update, remote.networkId, std::move(bytes));

		//Logger::logDev("Sending update " + entity.getName() + ": " + toString(size) + " bytes to peer " + toString(static_cast<int>(peerId)));
		//Logger::logDev("Update:\n" + EntityData(deltaData).toYAML() + "\n");
	}
}

void EntityNetworkRemotePeer::sendDestroyEntity(OutboundEntity& remote)
{
	allocatedOutboundIds.erase(remote.networkId);

	send(EntityNetworkHeaderType::Destroy, remote.networkId, Bytes());

	Logger::logDev("Sending destroy entity to peer " + toString(static_cast<int>(peerId)));
}

size_t EntityNetworkRemotePeer::send(EntityNetworkHeaderType type, EntityNetworkId networkId, Bytes data)
{
	// TODO: compress them into one update?

	const size_t size = data.size() + 3;
	
	auto packet = OutboundNetworkPacket(std::move(data));
	packet.addHeader(EntityNetworkEntityHeader(networkId));
	packet.addHeader(EntityNetworkHeader(type));
	parent->getSession().sendToPeer(packet, peerId);

	return size;
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

void EntityNetworkRemotePeer::onFirstDataBatchSent()
{
	if (parent->getSession().getType() == NetworkSessionType::Host) {
		auto packet = OutboundNetworkPacket(Bytes());
		packet.addHeader(EntityNetworkHeader(EntityNetworkHeaderType::ReadyToStart));
		parent->getSession().sendToPeer(packet, peerId);
	}
}
