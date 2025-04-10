#include "halley/net/entity/entity_network_remote_peer.h"
#include "halley/net/entity/entity_network_serialize.h"
#include "halley/net/entity/entity_network_session.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "halley/entity/data_interpolator.h"
#include "components/network_component.h"

#define USE_FAST_NETWORK_COMPONENT_UPDATES 1

using namespace Halley;

thread_local Bytes EntityNetworkRemotePeer::fastUpdateOutboundData;

EntityNetworkRemotePeer::EntityNetworkRemotePeer(EntityNetworkSession& parent, NetworkSession::PeerId peerId)
	: parent(&parent)
	, peerId(peerId)
{}

NetworkSession::PeerId EntityNetworkRemotePeer::getPeerId() const
{
	return peerId;
}

void EntityNetworkRemotePeer::sendEntities(Time t, gsl::span<const EntityNetworkUpdateInfo> entityIds, const EntityClientSharedData& clientData)
{
	Expects(isAlive());

	if (!isRemoteReady()) {
		if (timeSinceSend > maxSendInterval) {
			sendKeepAlive();
		}
		return;
	}

	timeSinceSend += t;
	
	// Mark all as not alive
	for (auto& e: outboundEntities) {
		e.second.alive = false;
	}

	Vector<EntityRef> toCreate;
	Vector<std::pair<EntityRef, OutboundEntity*>> toUpdate;

	for (auto entry: entityIds) {
		if (entry.ownerId == peerId) {
			// Don't send updates back to the owner
			continue;
		}

		const auto entity = parent->getWorld().getEntity(entry.entityId);
		if (parent->isEntityInView(entity, clientData, peerId)) {
			if (const auto iter = outboundEntities.find(entry.entityId); iter == outboundEntities.end()) {
				parent->setupOutboundInterpolators(entity);
				toCreate.push_back(entity);
			} else {
				iter->second.alive = true;
				toUpdate.emplace_back(entity, &iter->second);
			}
		}
	}

	// Order is important here, we need to first destroy, then update, then create
	// This is so we don't run into an issue where an entity is moved inside another and we attempt to create/update the new one while the old one is still present

	// Destroy dead entities
	for (auto& e: outboundEntities) {
		if (!e.second.alive) {
			sendDestroyEntity(e.second, e.first);
		}
	}

	// Update existing entities
	for (auto& [e, oe] : toUpdate) {
		sendUpdateEntity(t, *oe, e);
	}

	// Create new entities
	for (auto& e: toCreate) {
		if (e.hasParent()) {
			// NB: These checks defer create messages for child entities if the message to
			// create their parent entity has not been sent yet. Tries to avoid problems
			// with order of creation on the receiver side - there's code to attach children
			// to their parents post-creation, but that doesn't seem to resolve all our edge
			// cases.
			//
			// Simply skipping the sendCreateEntity() call works here because the alive check
			// will just pick them up again to be sent on the next update.
			if (const auto parent = e.getParent(); parent.getWorldPartition() == 0) {
				if (outboundEntities.find(parent.getEntityId()) == outboundEntities.end()) {
					continue;
				}
			}
		}
		sendCreateEntity(e);
	}

	std_ex::erase_if_value(outboundEntities, [](const OutboundEntity& e) { return !e.alive; });

	if (timeSinceSend > maxSendInterval) {
		sendKeepAlive();
	}
	
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
		// Don't destroy host entities. Host disconnecting means that the session is terminating, and destroying host entities could lead to bugs.
		if (parent->hasWorld() && peerId != 0) {
			for (const auto& [k, v] : inboundEntities) {
				destroyRemoteEntity(v.worldId);
			}
		}
		
		inboundEntities.clear();
		alive = false;
	}
}

bool EntityNetworkRemotePeer::hasJoinedWorld() const
{
	return joinedWorld;
}

void EntityNetworkRemotePeer::onJoinedWorld()
{
	joinedWorld = true;
}

void EntityNetworkRemotePeer::requestJoinWorld()
{
	send(EntityNetworkMessageJoinWorld());
}

void EntityNetworkRemotePeer::requestLobbyInfo()
{
	send(EntityNetworkMessageGetLobbyInfo());
}

void EntityNetworkRemotePeer::sendLobbyInfo(ConfigNode data)
{
	send(EntityNetworkMessageUpdateLobbyInfo(std::move(data)));
}

void EntityNetworkRemotePeer::setLobbyInfo(ConfigNode info)
{
	send(EntityNetworkMessageSetLobbyInfo(std::move(info)));
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
	//Logger::logDev("Send Create: " + entity.getName() + " (" + entity.getInstanceUUID() + ") to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B):\n" + deltaData.toYAML() + "\n");
	//Logger::logDev("Send Create: " + entity.getName() + " (" + entity.getInstanceUUID() +
	//	") with EntityNetworkId (" + result.networkId +
	//	") to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B)");

	send(EntityNetworkMessageCreate(result.networkId, std::move(bytes)));

	outboundEntities[entity.getEntityId()] = std::move(result);
}

void EntityNetworkRemotePeer::sendUpdateEntity(Time t, OutboundEntity& remote, EntityRef entity)
{
	remote.timeSinceSend += t;
	if (remote.timeSinceSend < parent->getMinSendInterval()) {
		return;
	}

	bool wantToLog = USE_FAST_NETWORK_COMPONENT_UPDATES && log;

#if USE_FAST_NETWORK_COMPONENT_UPDATES
    // Fast updates are possible only if a previous journal is available to compare to.
    bool canFastUpdate = !remote.fastUpdateJournal.empty();

    if (canFastUpdate) {
        Expects(parent->getEntitySerializationOptions().type == EntitySerialization::Type::Network);
        Expects(!parent->getEntitySerializationOptions().serializeAsStub);

        auto fastSerialize = EntityNetworkSerialize(parent->getResources(), parent->getByteDataInterpolatorSet());

    	if (fastSerialize.serializeEntityUpdate(entity, parent->getByteSerializationOptions())) {
    		bool modified = fastSerialize.processEntityUpdateChanges(remote.fastUpdateJournal);
    		bool modifiedInStructure = fastSerialize.hasEntityChanges();

    		wantToLog &= modified;

    		if (wantToLog) {
    			Logger::logInfo("Network update for entity " + entity.getName());
    		}

    		if (modified && !modifiedInStructure) {
    			remote.timeSinceSend = 0;

    			fastUpdateOutboundData.reserve(16384);
    			fastSerialize.getBytes(fastUpdateOutboundData, parent->getByteSerializationOptions(), wantToLog);
    			//Logger::logDev("Send Fast Update " + entity.getName() + " to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B)");

    			if (wantToLog) {
	        		Logger::logInfo("  - send fast serialize msg, " + toString(fastUpdateOutboundData.size()) + " bytes");
    			}

    			Bytes bytes(fastUpdateOutboundData);
    			send(EntityNetworkMessageUpdate(remote.networkId, std::move(bytes), true));
    		}

    		if (modifiedInStructure) {
    			canFastUpdate = false;
    			// Wipe the existing journal
    			remote.fastUpdateJournal.clear();
    		}
    	} else {
    		// Something went wrong, fall back to the slow path.
    		canFastUpdate = false;
    		remote.fastUpdateJournal.clear();
    		Logger::logWarning("Fast network serialize has failed, fall back using slow path");
    	}
    }
#else
    constexpr bool canFastUpdate = false;
#endif

    if (!canFastUpdate) {
        // Encode delta using interpolators
        auto newData = parent->getFactory().serializeEntity(entity, parent->getEntitySerializationOptions());
        auto retriever = DataInterpolatorSetRetriever(entity, true);
        auto options = parent->getEntityDeltaOptions();
        options.interpolatorSet = &retriever;
        auto deltaData = EntityDataDelta(remote.data, newData, options);

        if (deltaData.hasChange()) {
            remote.data = std::move(newData);
            remote.timeSinceSend = 0;

            auto bytes = Serializer::toBytes(deltaData, parent->getByteSerializationOptions());
            //Logger::logDev("Send Update " + entity.getName() + " to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B):\n" + deltaData.toYAML() + "\n");
            //Logger::logDev("Send Update " + entity.getName() + " to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B)");

        	if (wantToLog) {
        		Logger::logInfo("  - send EntityDataDelta, " + toString(bytes.size()) + " bytes");
        	}

            send(EntityNetworkMessageUpdate(remote.networkId, std::move(bytes), false));
        }

#if USE_FAST_NETWORK_COMPONENT_UPDATES
        // Binary serialization to (re-)build the update journal.
        auto serialize = EntityNetworkSerialize(parent->getResources(), parent->getByteDataInterpolatorSet());
        if (serialize.serializeEntityUpdate(entity, parent->getByteSerializationOptions())) {
	        serialize.processEntityUpdateChanges(remote.fastUpdateJournal);
        } else {
	        // If the fast update further above failed, for example because of a full journal,
        	// this one here will probably fail too - so we just drop the changes and retry
        	// next time.
    		remote.fastUpdateJournal.clear();
        }
#endif
    }
}

void EntityNetworkRemotePeer::sendDestroyEntity(OutboundEntity& remote, EntityId entityId)
{
	allocatedOutboundIds.erase(remote.networkId);

	send(EntityNetworkMessageDestroy(remote.networkId));

	//if (const auto entity = parent->getWorld().tryGetEntity(entityId); entity.isValid()) {
	//	Logger::logDev("Send Destroy: " + entity.getName() + " (" + entity.getInstanceUUID() +
	//		") with EntityNetworkId (" + remote.networkId +
	//		") to peer " + toString(static_cast<int>(peerId)));
	//}
}

void EntityNetworkRemotePeer::sendKeepAlive()
{
	send(EntityNetworkMessageKeepAlive());
}

void EntityNetworkRemotePeer::send(EntityNetworkMessage message)
{
	parent->sendToPeer(std::move(message), peerId);
	timeSinceSend = 0;
}

void EntityNetworkRemotePeer::receiveCreateEntity(const EntityNetworkMessageCreate& msg)
{
	const auto iter = inboundEntities.find(msg.entityId);
	if (iter != inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " already exists from peer " + toString(static_cast<int>(peerId)));
		return;
	}

	const auto delta = Deserializer::fromBytes<EntityDataDelta>(msg.bytes, parent->getByteSerializationOptions());
	if (!delta.getInstanceUUID()) {
		if (delta.getPrefab()) {
			Logger::logWarning("Unable to instantiate network entity, no instance UUID, prefab: " + delta.getPrefab());
		} else {
			Logger::logWarning("Unable to instantiate network entity, no instance UUID");
		}
		return;
	}

	auto [entityData, prefab, prefabUUID] = parent->getFactory().prefabDeltaToEntityData(delta, *delta.getInstanceUUID());
	if (!entityData) {
		Logger::logError("Unable to instantiate network entity");
		return;
	}

	if (entityData->getParentUUID().isValid()) {
		// The same check is done below, but the entity has already been created then.
		if (!parent->getWorld().findEntity(entityData->getParentUUID())) {
			Logger::logError("Parent " + toString(entityData->getParentUUID()) + " not found trying to instantiate network entity " + msg.entityId);
			return;
		}
	}

	auto [entity, parentUUID] = parent->getFactory().loadEntityDelta(delta, delta.getInstanceUUID(), EntitySerialization::makeMask(EntitySerialization::Type::SaveData, EntitySerialization::Type::Prefab, EntitySerialization::Type::Network));
	stripNestedNetworkComponents(entity);
	//Logger::logDev("Created entity " + entity.getName() + " with EntityNetworkId (" + toString(msg.entityId) + ") and EntityId (" + toString(entity.getEntityId()) + ") from network:\n\n" + EntityData(delta).toYAML());
	//Logger::logDev("Created entity " + entity.getName() + " with EntityNetworkId (" + toString(msg.entityId) + ") and EntityId (" + toString(entity.getEntityId()) + ") Instance UUID " + toString(entity.getInstanceUUID()));
	//if (entity.getParent().isValid()) {
	//	Logger::logDev("with parent " + entity.getParent().getName());
	//}

	if (parentUUID) {
		if (auto parentEntity = parent->getWorld().findEntity(parentUUID.value()); parentEntity) {
			entity.setParent(parentEntity.value());
		} else {
			Logger::logError("Parent " + toString(*parentUUID) + " not found for network entity \"" + entity.getName() + "\" (" + entity.getInstanceUUID() + ")");
		}
	}

	InboundEntity remote;
	remote.data = std::move(*entityData);
	remote.worldId = entity.getEntityId();
	inboundEntities[msg.entityId] = std::move(remote);

	auto& interpolatorSet = entity.setupNetwork(peerId);
	parent->onRemoteEntityCreated(entity, peerId);
	parent->requestSetupInterpolators(interpolatorSet, entity, true);

	auto& byteDataInterpolatorSet = entity.getComponent<NetworkComponent>().byteDataInterpolatorSet;
	parent->requestSetupByteDataInterpolators(byteDataInterpolatorSet, entity, true);
}

void EntityNetworkRemotePeer::receiveUpdateEntity(const EntityNetworkMessageUpdate& msg)
{
	const auto iter = inboundEntities.find(msg.entityId);
	if (iter == inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " not found from peer " + toString(static_cast<int>(peerId)));
		return;
	}
	auto& remote = iter->second;

	auto entity = parent->getWorld().tryGetEntity(remote.worldId);
	if (!entity.isValid()) {
		Logger::logWarning("Entity with network id (" + toString(static_cast<int>(msg.entityId)) + ") and EntityId (" + toString(remote.worldId) + ") not alive in the world from peer " + toString(static_cast<int>(peerId)));
		if (!msg.fastSerialize) {
			const auto delta = Deserializer::fromBytes<EntityDataDelta>(msg.bytes, parent->getByteSerializationOptions());
			Logger::logWarning("Caused by trying to update entity:\n" + delta.toYAML());
		}
		return;
	}

    if (msg.fastSerialize) {
        //Logger::logDev("Receive Fast Update " + entity.getName() + " (" + toString(msg.bytes.size()) + " B)");

        auto serialize = EntityNetworkSerialize(parent->getResources(), parent->getByteDataInterpolatorSet());

        try {
            serialize.deserializeEntityUpdate(entity, entity.getPrefab(), msg.bytes, parent->getByteSerializationOptions());
			// TODO: is this still required?
        	stripNestedNetworkComponents(entity);
        } catch (const std::exception& e) {
            Logger::logError("Exception while processing update entity from network");
            Logger::logException(e);
        }
    } else {
        const auto delta = Deserializer::fromBytes<EntityDataDelta>(msg.bytes, parent->getByteSerializationOptions());

        auto retriever = DataInterpolatorSetRetriever(entity, false);
        //Logger::logDev("Receive Update " + entity.getName() + " (" + toString(msg.bytes.size()) + " B)");
        //Logger::logDev("Updating entity " + entity.getName() + ":\n" + delta.toYAML());

        try {
            parent->getFactory().updateEntity(entity, delta,
                                              EntitySerialization::makeMask(EntitySerialization::Type::Network),
                                              nullptr, &retriever);
            stripNestedNetworkComponents(entity);
        } catch (const std::exception &e) {
            Logger::logError("Exception while processing update entity from network:\n" + delta.toYAML());
            Logger::logException(e);
        }
        remote.data.applyDelta(delta);
    }
}

void EntityNetworkRemotePeer::receiveDestroyEntity(const EntityNetworkMessageDestroy& msg)
{
	const auto iter = inboundEntities.find(msg.entityId);
	if (iter == inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " not found from peer " + toString(static_cast<int>(peerId)));
		return;
	}
	auto& remote = iter->second;

	//if (const auto entityRef = parent->getWorld().tryGetEntity(remote.worldId); entityRef.isValid()) {
	//	Logger::logDev("Destroying from network: " + entityRef.getName() + " UUID " + toString(entityRef.getInstanceUUID()) + " NetworkEntityId (" + toString(static_cast<int>(msg.entityId)) + ") and EntityId(" + toString(remote.worldId) + ")");
	//}

	destroyRemoteEntity(remote.worldId);

	inboundEntities.erase(msg.entityId);
}

void EntityNetworkRemotePeer::destroyRemoteEntity(EntityId id)
{
	auto entity = parent->getWorld().tryGetEntity(id);
	if (entity.isValid()) {
		bool hasBeenAssignedToHost = false;
		if (const auto networkComponent = entity.tryGetComponent<NetworkComponent>(); networkComponent && networkComponent->creatorId) {
			// Checks if the entity was created locally, and network ownership assigned to host.
			// In this case, do not destroy the entity. See NetworkSendSystem::update().
			hasBeenAssignedToHost = networkComponent->creatorId != networkComponent->ownerId;
		}

		entity.setFromNetwork(false);

		if (!hasBeenAssignedToHost) {
			parent->getWorld().destroyEntity(entity);
		} else {
			//Logger::logDev("Network ownership for local entity was auto-assigned to host, ignoring destroy msg.");
		}
	} else {
		Logger::logWarning("Network entity has gone missing.");
	}
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

void EntityNetworkRemotePeer::stripNestedNetworkComponents(EntityRef entity, int depth)
{
	if (depth > 0) {
		if (entity.hasComponent<NetworkComponent>()) {
			entity.removeComponent<NetworkComponent>();
		}
	}

	for (auto c: entity.getChildren()) {
		stripNestedNetworkComponents(c, depth + 1);
	}
}

EntityId EntityNetworkRemotePeer::findInboundEntity(EntityNetworkId networkId) const
{
	if (inboundEntities.contains(networkId)) {
		return inboundEntities.at(networkId).worldId;
	}
	return {};
}

EntityId EntityNetworkRemotePeer::findOutboundEntity(EntityNetworkId networkId) const
{
	for (const auto& e: outboundEntities) {
		if (e.second.networkId == networkId) {
			return e.first;
		}
	}
	return {};
}

void EntityNetworkRemotePeer::logUpdates()
{
	log = !log;
}
