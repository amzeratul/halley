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

	if (!isRemoteReady()) {
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

void EntityNetworkRemotePeer::receiveNetworkMessage(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg)
{
	Expects(isAlive());

	if (msg.getType() == EntityNetworkHeaderType::Create) {
		receiveCreateEntity(msg.getMessage<EntityNetworkMessageCreate>());
	} else if (msg.getType() == EntityNetworkHeaderType::Update) {
		receiveUpdateEntity(msg.getMessage<EntityNetworkMessageUpdate>());
	} else if (msg.getType() == EntityNetworkHeaderType::Destroy) {
		receiveDestroyEntity(msg.getMessage<EntityNetworkMessageDestroy>());
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
	auto bytes = Serializer::toBytes(deltaData, parent->getByteSerializationOptions());
	send(EntityNetworkMessageCreate(result.networkId, std::move(bytes)));
	
	outboundEntities[entity.getEntityId()] = std::move(result);

	//Logger::logDev("Sending create " + entity.getName() + " (" + entity.getInstanceUUID() + ") to peer " + toString(static_cast<int>(peerId)));
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
		send(EntityNetworkMessageUpdate(remote.networkId, std::move(bytes)));

		//Logger::logDev("Sending update " + entity.getName() + ": " + toString(size) + " bytes to peer " + toString(static_cast<int>(peerId)));
		//Logger::logDev("Update:\n" + EntityData(deltaData).toYAML() + "\n");
	}
}

void EntityNetworkRemotePeer::sendDestroyEntity(OutboundEntity& remote)
{
	allocatedOutboundIds.erase(remote.networkId);

	send(EntityNetworkMessageDestroy(remote.networkId));

	//Logger::logDev("Sending destroy entity to peer " + toString(static_cast<int>(peerId)));
}

void EntityNetworkRemotePeer::send(EntityNetworkMessage message)
{
	parent->sendToPeer(std::move(message), peerId);
}

void EntityNetworkRemotePeer::receiveCreateEntity(const EntityNetworkMessageCreate& msg)
{
	const auto iter = inboundEntities.find(msg.entityId);
	if (iter != inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " already exists from peer " + toString(static_cast<int>(peerId)));
		return;
	}

	const auto delta = Deserializer::fromBytes<EntityDataDelta>(msg.bytes, parent->getByteSerializationOptions());
	//Logger::logDev("Instantiating from network:\n\n" + EntityData(delta).toYAML());

	auto [entityData, prefab, prefabUUID] = parent->getFactory().prefabDeltaToEntityData(delta);
	auto [entity, parentUUID] = parent->getFactory().loadEntityDelta(delta, {});

	if (parentUUID) {
		if (auto parentEntity = parent->getWorld().findEntity(parentUUID.value()); parentEntity) {
			entity.setParent(parentEntity.value());
		} else {
			Logger::logError("Parent " + toString(*parentUUID) + " not found for network entity \"" + entity.getName() + "\"");
		}
	}

	entity.setupNetwork(peerId);
	
	InboundEntity remote;
	remote.data = std::move(entityData);
	remote.worldId = entity.getEntityId();

	inboundEntities[msg.entityId] = std::move(remote);

	parent->onRemoteEntityCreated(entity, peerId);
}

void EntityNetworkRemotePeer::receiveUpdateEntity(const EntityNetworkMessageUpdate& msg)
{
	const auto iter = inboundEntities.find(msg.entityId);
	if (iter == inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " not found from peer " + toString(static_cast<int>(peerId)));
		return;
	}
	auto& remote = iter->second;

	auto entity = parent->getWorld().getEntity(remote.worldId);
	if (!entity.isValid()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " not alive in the world from peer " + toString(static_cast<int>(peerId)));
		return;
	}
	
	auto delta = Deserializer::fromBytes<EntityDataDelta>(msg.bytes, parent->getByteSerializationOptions());

	parent->getFactory().updateEntity(entity, delta, static_cast<int>(EntitySerialization::Type::SaveData));
	remote.data.applyDelta(delta);
}

void EntityNetworkRemotePeer::receiveDestroyEntity(const EntityNetworkMessageDestroy& msg)
{
	const auto iter = inboundEntities.find(msg.entityId);
	if (iter == inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " not found from peer " + toString(static_cast<int>(peerId)));
		return;
	}
	auto& remote = iter->second;

	parent->getWorld().destroyEntity(remote.worldId);

	inboundEntities.erase(msg.entityId);
}

bool EntityNetworkRemotePeer::isRemoteReady() const
{
	auto& sharedData = parent->getSession().getClientSharedData<EntityClientSharedData>(peerId);
	return !!sharedData.viewRect;
}

void EntityNetworkRemotePeer::onFirstDataBatchSent()
{
	if (parent->getSession().getType() == NetworkSessionType::Host) {
		send(EntityNetworkMessage(EntityNetworkMessageReadyToStart()));
	}
}
