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

void EntityNetworkRemotePeer::sendEntities(Time t, uint8_t myPeerId, gsl::span<const EntityNetworkUpdateInfo> entityIds, const EntityClientSharedData& clientData)
{
	Expects(isAlive());
	Expects(myPeerId != peerId);

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

	for (const auto& entry: entityIds) {
		if (entry.ownerId == peerId && entry.authorityId == peerId) {
			// Don't send updates to the owner, if it has authority.
			continue;
		}

		const auto entity = parent->getWorld().getEntity(entry.entityId);

		if (entry.ownerId == peerId && entry.authorityId == myPeerId) {
			// Owned by remote peer, but authority has been given to local peer. We want to create
			// some outbound entity that can be used to send updates, until the authority is given
			// back.
			//
			// This outbound entity isn't created here, but in prepareChangeEntityAuthority().
			//
			// TODO: auto-release authority if goes out of view?
			if (const auto iter = outboundEntities.find(entry.entityId); iter != outboundEntities.end()) {
				// Has an outbound entity assigned. Keep it alive and updating.
				Expects(iter->second.hasAuthorityOnly);
				iter->second.alive = true;
				toUpdate.emplace_back(entity, &iter->second);
			} else {
				Logger::logWarning("No temporary outbound entity found for " + toString(entry.entityId & 0xffffffff));
			}
			continue;
		}

		if (entry.ownerId == myPeerId && entry.authorityId != myPeerId) {
			// Owned by host/this local peer, but authority has been transferred. There should be
			// a regular outbound entity.
			//
			// We don't want to send updates to the peer who took authority, but we want to keep
			// this outbound entity alive until authority is given back.
			if (const auto iter = outboundEntities.find(entry.entityId); iter != outboundEntities.end()) {
				Expects(!iter->second.hasAuthorityOnly);
				iter->second.alive = true;
				if (entry.authorityId != peerId) {
					toUpdate.emplace_back(entity, &iter->second);
				}
			} else {
				Logger::logError("No outbound entity found that is owned by local peer");
			}
			continue;
		}

		if (parent->isEntityInView(entity, clientData, peerId)) {
			if (const auto iter = outboundEntities.find(entry.entityId); iter == outboundEntities.end()) {
				parent->setupOutboundInterpolators(entity);
				toCreate.push_back(entity);
			} else {
				Expects(!iter->second.hasAuthorityOnly);
				iter->second.alive = true;
				toUpdate.emplace_back(entity, &iter->second);
			}
		}
	}

	// Order is important here, we need to first destroy, then update, then create
	// This is so we don't run into an issue where an entity is moved inside another, and we
	// attempt to create/update the new one while the old one is still present.

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
	for (const auto& e: toCreate) {
		if (e.hasParent()) {
			// NB: These checks defer create messages for child entities if the message to
			// create their parent entity has not been sent yet. Tries to avoid problems
			// with order of creation on the receiver side - there's code to attach children
			// to their parents post-creation, but that doesn't seem to resolve all our edge
			// cases.
			//
			// This must NOT be done for non-networked parent entities.
			//
			// Simply skipping the sendCreateEntity() call works here because the alive check
			// will just pick them up again to be sent on the next update.
			if (const auto parent = e.getParent(); parent.hasComponentInAncestors<NetworkComponent>()) {
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
	Expects(fromPeerId == peerId);

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
				destroyRemoteEntity(v.worldId, v.debugName);
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

void EntityNetworkRemotePeer::sendCreateEntity(const EntityRef& entity)
{
	OutboundEntity result;

	result.networkId = assignId();

	result.data = parent->getFactory().serializeEntity(entity, parent->getEntitySerializationOptions());

	if (!entity.getPrefab() && entity.getWorldPartition() != 0) {
		// This is loaded as part of a world chunk - peers already got all the data, they only
		// need to know about the networkId assigned.
		auto bytes = Serializer::toBytes(entity.getInstanceUUID(), parent->getByteSerializationOptions());
		send(EntityNetworkMessageCreate(result.networkId, bytes, true));
	} else {
		auto deltaData = parent->getFactory().entityDataToPrefabDelta(result.data, entity.getPrefab(), parent->getEntityDeltaOptions());

		auto bytes = Serializer::toBytes(deltaData, parent->getByteSerializationOptions());
		//Logger::logDev("Send Create: " + entity.getName() + " (" + entity.getInstanceUUID() + ") to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B):\n" + deltaData.toYAML() + "\n");
		//Logger::logDev("Send Create: " + entity.getName() + " (" + entity.getInstanceUUID() +
		//	") with EntityNetworkId (" + result.networkId +
		//	") to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B)");

		send(EntityNetworkMessageCreate(result.networkId, std::move(bytes), false));
	}

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
    // Fast updates are possible only if a previous journal is available to compare to,
    // or if it's an outbound entity marked as "for changed authority".
    bool canFastUpdate = !remote.fastUpdateJournal.empty() || remote.hasAuthorityOnly;

    if (canFastUpdate) {
        Expects(parent->getEntitySerializationOptions().type == EntitySerialization::Type::Network);
        Expects(!parent->getEntitySerializationOptions().serializeAsStub);

        auto fastSerialize = EntityNetworkSerialize(parent);

    	if (fastSerialize.serializeEntityUpdate(entity, parent->getByteSerializationOptions())) {
    		bool modified = false;
    		bool modifiedInStructure = false;

    		if (remote.fastUpdateJournal.empty()) {
    			// This must be an outbound entity with "hasAuthorityOnly". If it just has been
    			// created, its previous journal is still empty.
    			Expects(remote.hasAuthorityOnly);
    			// Just process and store the journal, but keep marked as "not modified".
		        fastSerialize.processEntityUpdateChanges(remote.fastUpdateJournal);
    			// TODO: can this miss some intermediate, local modifications?
    			Logger::logDev("populating outbound entity journal, authority-only");
    		} else {
    			modified = fastSerialize.processEntityUpdateChanges(remote.fastUpdateJournal);
    			modifiedInStructure = fastSerialize.hasEntityChanges(entity, wantToLog);
    		}

    		wantToLog &= modified;

    		if (wantToLog) {
    			Logger::logInfo("Network update for entity " + entity.getName());
    		}

    		if (modified && !modifiedInStructure) {
    			remote.timeSinceSend = 0;

    			fastUpdateOutboundData.reserve(EntityNetworkSerialize::getBytesCapacity());
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
	    		Logger::logWarning("Network entity " + entity.getName() + " has been modified in structure, fall back using slow path");
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
    	if (remote.hasAuthorityOnly) {
            Logger::logError("Full network updates unsupported for entities with changed authority");
    		return;
    	}

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
        auto serialize = EntityNetworkSerialize(parent);
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
	if (remote.hasAuthorityOnly) {
		// Don't want (and not allowed to) destroy this here.
		Logger::logError("Attempt to destroy temporary outbound entity");
		return;
	}

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

	if (msg.assignNetworkIdOnly) {
		const auto uuid = Deserializer::fromBytes<UUID>(msg.bytes, parent->getByteSerializationOptions());
		auto entity = parent->getWorld().findEntity(uuid);

		if (!entity || !entity->isValid()) {
			Logger::logWarning("Unable to assign network entity, no instance found with UUID " + toString(uuid));
			return;
		}

		InboundEntity remote;
		// Construct entity data from local entity, since the host doesn't send any delta in this case.
		remote.data = parent->getFactory().serializeEntity(*entity, parent->getEntitySerializationOptions());;
		remote.worldId = entity->getEntityId();
		remote.debugName = entity->getName();
		inboundEntities[msg.entityId] = std::move(remote);

		auto& interpolatorSet = entity->setupNetwork(peerId);
		parent->onRemoteEntityCreated(*entity, peerId);
		parent->requestSetupInterpolators(interpolatorSet, *entity, true);

		auto& byteDataInterpolatorSet = entity->getComponent<NetworkComponent>().byteDataInterpolatorSet;
		parent->requestSetupByteDataInterpolators(byteDataInterpolatorSet, *entity, true);

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
	/*if (entity.getName().contains("tree_linden_")) {
		Logger::logDev("Created entity " + entity.getName() + " with EntityNetworkId (" + toString(msg.entityId) + ") and EntityId (" + toString(entity.getEntityId()) + ") Instance UUID " + toString(entity.getInstanceUUID()));
		if (entity.getParent().isValid()) {
			Logger::logDev("with parent " + entity.getParent().getName());
		}
	}*/

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
	remote.debugName = entity.getName() + "|" + entity.getPrefabAssetId();
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

        auto serialize = EntityNetworkSerialize(parent);

        try {
            serialize.deserializeEntityUpdate(entity, entity.getPrefab(), msg.bytes, parent->getByteSerializationOptions());
			// TODO: is this still required?
        	stripNestedNetworkComponents(entity);
        } catch (const std::exception& e) {
            Logger::logError("Exception while processing update entity from network");
            Logger::logException(e);
        }
    } else {
    	if (iter->second.forChangedAuthorityOnly) {
            Logger::logError("Full network updates unsupported for temporary inbound entities");
    		return;
    	}

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

	destroyRemoteEntity(remote.worldId, remote.debugName);

	inboundEntities.erase(msg.entityId);
}

void EntityNetworkRemotePeer::destroyRemoteEntity(EntityId id, const String& debugName)
{
	auto entity = parent->getWorld().tryGetEntity(id);
	if (entity.isValid()) {
		bool shouldBeDeleted = entity.getWorldPartition() == 0;
		if (!shouldBeDeleted && entity.getPrefab()) {
			// Don't delete non-prefab world entities.
			// See sendCreateEntity() for the correlating check on network entity creation.
			bool hasBeenAssignedToHost = false;
			if (const auto networkComponent = entity.tryGetComponent<NetworkComponent>(); networkComponent && networkComponent->creatorId) {
				// Checks if the entity was created locally, but network ownership assigned to host.
				hasBeenAssignedToHost = networkComponent->creatorId != networkComponent->ownerId;
			}
			shouldBeDeleted = hasBeenAssignedToHost;
		}

		entity.setFromNetwork(false);

		if (shouldBeDeleted) {
			parent->getWorld().destroyEntity(entity);
		} else {
			//Logger::logDev("Network ownership for local entity was auto-assigned to host, ignoring destroy msg.");
		}
	} else if (parent->isHost()) {
		// NB: This can be very legit for peers.
		// Updates can arrive after a chunk has been unloaded.
		Logger::logWarning("Network entity has gone missing: " + debugName);
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

void EntityNetworkRemotePeer::prepareChangeEntityAuthority(EntityId entityId, NetworkSession::PeerId myPeerId,
	NetworkSession::PeerId ownerId, std::optional<NetworkSession::PeerId> authorityId)
{
	if (authorityId.has_value()) {
		if (myPeerId == ownerId) {
			// I lose authority. Create a temporary inbound entity.
			//Logger::logDev("Lost authority of " + toString(entityId.value & 0xffffffff) + " to " + toString((int) authorityId.value()));

			// There should be some outbound entity available.
			if (outboundEntities.contains(entityId)) {
				const auto &oe = outboundEntities[entityId];

				InboundEntity remote;
				remote.worldId = entityId;
				remote.debugName = "[temp authority]";
				remote.forChangedAuthorityOnly = true;

				if (!inboundEntities.contains(oe.networkId)) {
					inboundEntities[oe.networkId] = std::move(remote);
				} else {
					Logger::logWarning("Entity with network id " + toString(static_cast<int>(oe.networkId)) + " already has an inbound entity");
				}
			} else {
				Logger::logWarning("No outbound entity found to create temporary inbound entity from");
			}
		} else if (myPeerId == authorityId) {
			// I'm taking authority. Create a temporary outbound entity.
			//Logger::logDev("Took authority of " + toString(entityId.value & 0xffffffff) + " from " + toString((int) ownerId));

			// Search for inbound entity.
			const auto inboundIter = std_ex::find_if(inboundEntities, [&](const auto& kv) {
				return kv.second.worldId == entityId;
			});

			if (inboundIter != inboundEntities.end()) {
				OutboundEntity outbound = {};
				outbound.alive = true;
				outbound.hasAuthorityOnly = true;
				outbound.networkId = inboundIter->first;

				if (!outboundEntities.contains(entityId)) {
					outboundEntities[entityId] = std::move(outbound);
				} else {
					Logger::logWarning("Entity id " + toString(static_cast<int>(entityId)) + " already has an outbound entity");
				}
			} else {
				Logger::logWarning("No inbound entity found to create temporary outbound entity from");
			}
		}
	} else {
		if (myPeerId == ownerId) {
			// I've been given back authority. Remove the temporary inbound entity.
			//Logger::logDev("Recovered authority of " + toString(entityId.value & 0xffffffff));
			const size_t found = std_ex::erase_if_value(inboundEntities, [&](const auto &value) {
				if (value.worldId == entityId) {
					Expects(value.forChangedAuthorityOnly);
					return true;
				}
				return false;
			});
			if (found != 1) {
				Logger::logWarning(toString(found) + " temporary inbound entities deleted, should be 1");
			}
		} else {
			// I'm returning authority to owner. Remove the temporary outbound entity.
			//Logger::logDev("Gave back authority of " + toString(entityId.value & 0xffffffff) + " to " + toString((int) ownerId));
			if (outboundEntities.contains(entityId)) {
				outboundEntities.erase(entityId);
			} else {
				Logger::logWarning("No temporary outbound entity found for deletion");
			}
		}
	}
}
