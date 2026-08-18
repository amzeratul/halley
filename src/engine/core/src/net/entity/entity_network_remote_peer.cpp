#include "halley/net/entity/entity_network_remote_peer.h"

#include "halley/net/entity/entity_network_session.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "halley/net/interpolators/data_interpolator.h"
#include "components/network_component.h"
#include "halley/entity/components/transform_2d_component.h"

#define USE_FAST_NETWORK_COMPONENT_UPDATES 1
#define WAIT_UNTIL_DORMANT_AFTER_FRAME_MODIFIED 2.0

// Limits the number of "entity create" messages sent per frame, per peer.
// This is a preventive measure to avoid congestion in AckUnreliableConnection.
//
// Above that limit, the host won't create outbound entities, and simply try again
// the following frame(s) - as long as the entities are still in view.
#define MAX_SEND_CREATE_PER_FRAME 96

using namespace Halley;

Serializer& EntityNetworkInstanceInfo::serialize(Serializer& s) const
{
	s << instanceUUID;
	s << parentUUID;
	return s;
}

Deserializer& EntityNetworkInstanceInfo::deserialize(Deserializer& s)
{
	s >> instanceUUID;
	s >> parentUUID;
	return s;
}

EntityNetworkRemotePeer::EntityNetworkRemotePeer(EntityNetworkSession& parentSession, NetworkSession::PeerId peerId)
	: parentSession(&parentSession)
	, peerId(peerId)
{}

NetworkSession::PeerId EntityNetworkRemotePeer::getPeerId() const
{
	return peerId;
}

SendEntitiesStats EntityNetworkRemotePeer::sendEntities(Time t, uint8_t myPeerId, gsl::span<const EntityNetworkUpdateInfo> entityIds, const EntityClientSharedData& clientData)
{
	HalleyAssertDev(isAlive());
	HalleyAssertDev(myPeerId != peerId);

	SendEntitiesStats stats;

	if (!isRemoteReady()) {
		if (timeSinceSend > maxSendInterval) {
			sendKeepAlive();
		}
		return stats;
	}

	timeSinceSend += t;

	// Timestamp entity updates with the "network time" we estimate the remote peer is at right now.
	int32_t sessionTimestamp = parentSession->getSession().getPeerSessionTimeMs(peerId);

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

		const auto entity = parentSession->getWorld().getEntity(entry.entityId);

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
				HalleyAssertDev(iter->second.hasAuthorityOnly);
				iter->second.alive = true;
				iter->second.requiresEntityFrameModified = entry.requiresEntityFrameModified;
				toUpdate.emplace_back(entity, &iter->second);
				++stats.nCheckedAcquiredAuthority;
			} else {
				Logger::logWarning("No temporary outbound entity found for " + entity.getName() + ", " +
					entity.getInstanceUUID(), true);
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
				HalleyAssertDev(!iter->second.hasAuthorityOnly);
				iter->second.alive = true;
				if (entry.authorityId != peerId) {
					iter->second.requiresEntityFrameModified = entry.requiresEntityFrameModified;
					toUpdate.emplace_back(entity, &iter->second);
					++stats.nCheckedRelinquishedAuthority;
				}
				continue;
			} else {
				// No entry found for this entity in outboundEntities. This usually means that this
				// particular peer is not in range.
				if (entry.authorityId == peerId) {
					Logger::logError("No outbound entity found for " + toString(entity.getName()) + ", " +
						entity.getInstanceUUID() + " with authority grabbed by " + toString(static_cast<int>(entry.authorityId)), true);
				}
			}
		}

		if (entry.alwaysSend || parentSession->isEntityInView(entity, clientData, peerId)) {
			++stats.nCheckedRegular;
			if (const auto iter = outboundEntities.find(entry.entityId); iter == outboundEntities.end()) {
				toCreate.push_back(entity);
			} else {
				HalleyAssertDev(!iter->second.hasAuthorityOnly);
				iter->second.alive = true;
				iter->second.requiresEntityFrameModified = entry.requiresEntityFrameModified;
				toUpdate.emplace_back(entity, &iter->second);
			}
		}
	}

	// Order is important here, we need to first destroy, then update, then create
	// This is so we don't run into an issue where an entity is moved inside another, and we
	// attempt to create/update the new one while the old one is still present.

	// Destroy dead entities
	for (auto& e: outboundEntities) {
		if (e.second.alive) {
			continue;
		}

		if (e.second.forChildEntityTemporaryOnly) {
			// Keep temporary outbound entities alive for child entities with changed authority.
			// They are removed if authority is given back, see prepareChangeEntityAuthority().
			e.second.alive = true;
			continue;
		}

		sendDestroyEntity(e.second, e.first);
		++stats.nDestroyed;
	}

	// Update existing entities
	for (auto& [e, oe] : toUpdate) {
		sendUpdateEntity(t, sessionTimestamp, *oe, e, stats);
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
		parentSession->setupOutboundInterpolators(e);
		sendCreateEntity(e);
		++stats.nCreated;

		if (stats.nCreated >= MAX_SEND_CREATE_PER_FRAME) {
			// Stop sending for this frame.
			//Logger::logDev("Sent create for " + toString(stats.nCreated) + "/" + toCreate.size());
			break;
		}
	}

	std_ex::erase_if_value(outboundEntities, [](const OutboundEntity& e) { return !e.alive; });

	if (timeSinceSend > maxSendInterval) {
		sendKeepAlive();
	}
	
	if (!hasSentData) {
		hasSentData = true;
		onFirstDataBatchSent();
	}

	return stats;
}

void EntityNetworkRemotePeer::receiveNetworkMessage(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg)
{
	HalleyAssertDev(isAlive());
	HalleyAssertDev(fromPeerId == peerId);

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
		if (parentSession->hasWorld() && peerId != 0) {
			for (const auto& [k, v] : inboundEntities) {
				destroyRemoteEntity(v);
			}
		}
		
		inboundEntities.clear();
		tempInboundEntities.clear();
		alive = false;
	}
}

void EntityNetworkRemotePeer::update(Time dt)
{
	trySpawningPendingEntities();
	interpolateRemoteEntityPositions(dt);
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

	result.data = parentSession->getFactory().serializeEntity(entity, parentSession->getEntitySerializationOptions());

	const bool assignNetworkIdOnly = entity.getWorldPartition() != 0 && !entity.getPrefab();

	if (assignNetworkIdOnly) {
		// This is loaded as part of a world chunk - peers already got all the data, they only
		// need to know about the networkId assigned.
		EntityNetworkInstanceInfo info = { entity.getInstanceUUID(), {} };
		if (entity.hasParent()) {
			info.parentUUID = entity.getParent().getInstanceUUID();
		}

		auto bytes = Serializer::toBytes(info, parentSession->getByteSerializationOptions());
		send(EntityNetworkMessageCreate(result.networkId, std::move(bytes), entity.getWorldPartition(), true));

		// Need to flag this one though, so that any host-side changes are sent through a first update.
		// Without this, the peer wouldn't be notified about any modifications until the next actual change.
		result.forceNextFastUpdate = true;
	} else {
		auto deltaData = parentSession->getFactory().entityDataToPrefabDelta(result.data, entity.getPrefab(), parentSession->getEntityDeltaOptions());

		auto bytes = Serializer::toBytes(deltaData, parentSession->getByteSerializationOptions());
		//Logger::logDev("Send Create: " + entity.getName() + " (" + entity.getInstanceUUID() + ") to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B):\n" + deltaData.toYAML() + "\n");
		//Logger::logDev("Send Create: " + entity.getName() + " (" + entity.getInstanceUUID() +
		//	") with EntityNetworkId (" + result.networkId +
		//	") to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B)");

		send(EntityNetworkMessageCreate(result.networkId, std::move(bytes), entity.getWorldPartition(), false));
	}

	outboundEntities[entity.getEntityId()] = std::move(result);
}

void EntityNetworkRemotePeer::sendUpdateEntity(Time t, int32_t sessionTimestamp, OutboundEntity& remote, const EntityRef& entity, SendEntitiesStats& stats)
{
	const Time minSendInterval = parentSession->getMinSendInterval();

	remote.timeSinceSend += t;
	if (remote.timeSinceSend < minSendInterval) {
		// These updates are just to keep the stats counters from oscillating too much.
		if (remote.requiresEntityFrameModified) {
			++stats.nUpdateIdle;
		} else {
			++stats.nUpdateSameHash;
		}
		return;
	}

	remote.timeSinceSend = 0;

#if defined(DEV_BUILD) && defined(_WIN32)
	static constexpr bool checkExpectNoUpdate = true;
	static constexpr bool checkExpectSameHash = false; // very costly, should be off by default
#else
	static constexpr bool checkExpectNoUpdate = false;
	static constexpr bool checkExpectSameHash = false;
#endif

	// For entities marked as such, check the "last frame modified" counter. If they match, skip
	// updates (to avoid costly operations to serialize and compare entity/component changes).
	bool expectNoUpdate = false;

	if (remote.requiresEntityFrameModified) {
		uint32_t frameIdx = entity.getLastFrameModified();
		if (remote.frameModifiedIdx != frameIdx) {
			// Counter has been changed.
			remote.frameModifiedIdx = frameIdx;
			remote.waitAfterFrameModified = WAIT_UNTIL_DORMANT_AFTER_FRAME_MODIFIED;
		} else {
			if (remote.waitAfterFrameModified > 0) {
				// Wait for a little while until we stop checking.
				remote.waitAfterFrameModified -= minSendInterval;
			} else {
				++stats.nUpdateIdle;
				if (checkExpectNoUpdate) {
					expectNoUpdate = true;
				} else {
					return;
				}
			}
		}
#if defined(DEV_BUILD) && defined(_WIN32)
	} else {
		// Dev build only: check if requiresEntityFrameModified is not set, but someone called
		// setLastFrameModified(). This isn't a problem, but can pinpoint a potential optimization.
		uint32_t frameIdx = entity.getLastFrameModified();
		if (remote.frameModifiedIdx != frameIdx) {
			remote.frameModifiedIdx = frameIdx;
			Logger::logDev("Manual network update notify for " + entity.getEntityId().toDetailedString() + ", " + entity.getName() + ", " +
				entity.getPrefabAssetId().value_or("(no prefab)") + ", but 'requiresEntityFrameModified' not set", true);
		}
#endif
	}

	bool wantToLog = USE_FAST_NETWORK_COMPONENT_UPDATES && log;

#if USE_FAST_NETWORK_COMPONENT_UPDATES
	fastSerializer.setSession(parentSession);

	// Serialize entity, using fast path, to compute a content hash. Early-out if the hash did not change.
	bool foundSameHash = false;

	const uint64_t contentHash = fastSerializer.serializeEntityHash(entity, parentSession->getByteSerializationOptions(), checkExpectSameHash);
	if (contentHash == remote.lastSerializerHash) {
		if (!expectNoUpdate) {
			++stats.nUpdateSameHash;
			//Logger::logDev("Checking fast: " + entity.getName() + " " + entity.getEntityId().toDetailedString(), true);
		}
		if (checkExpectSameHash) {
			foundSameHash = true;
		} else {
			return;
		}
	}
	uint64_t prevHashRemoveMe = remote.lastSerializerHash;
	remote.lastSerializerHash = contentHash;

	// Fast updates are possible only if a previous journal is available to compare to,
    // or if it's an outbound entity marked as "for changed authority".
    bool canFastUpdate = !remote.fastUpdateJournal.empty() || remote.hasAuthorityOnly;

	if (!expectNoUpdate) {
		++stats.nUpdateChecked;
		//Logger::logDev("Checking full: " + entity.getName() + " " + entity.getEntityId().toDetailedString(), true);
	}

    if (canFastUpdate) {
        HalleyAssertDev(parentSession->getEntitySerializationOptions().type == EntitySerialization::Type::Network);
        HalleyAssertDev(!parentSession->getEntitySerializationOptions().serializeAsStub);

    	if (fastSerializer.serializeEntityUpdate(entity, parentSession->getByteSerializationOptions())) {
    		bool modified = false;
    		bool modifiedInStructure = false;

    		if (remote.fastUpdateJournal.empty()) {
    			// This must be an outbound entity with "hasAuthorityOnly". If it just has been
    			// created, its previous journal is still empty.
    			HalleyAssertDev(remote.hasAuthorityOnly);
    			// Just process and store the journal, but keep marked as "not modified" ...
		        fastSerializer.processEntityUpdateChanges(remote.fastUpdateJournal, false);
    			// ... but set flag to force an update next tick, so we don't miss any changes.
    			remote.forceNextFastUpdate = true;
    			//Logger::logDev("populating outbound entity journal, authority-only, for " + entity.getName());
    		} else {
    			modified = fastSerializer.processEntityUpdateChanges(remote.fastUpdateJournal, remote.forceNextFastUpdate);
    			modifiedInStructure = fastSerializer.hasEntityChanges(entity, wantToLog);

    			// If the forceNextFastUpdate flag is set, mark as modified and to be sent, even if
    			// there is no change.
    			if (remote.forceNextFastUpdate) {
    				HalleyAssertDebug(modified);
    				remote.forceNextFastUpdate = false;
    			}
    		}

    		wantToLog &= modified;

    		if (wantToLog) {
    			Logger::logInfo("Network update for entity " + entity.getName());
    		}

    		if (modified && !modifiedInStructure) {
    			fastUpdateOutboundData.reserve(fastSerializer.getBytesCapacity());
    			size_t outboundDataSize = fastSerializer.getBytes(fastUpdateOutboundData, parentSession->getByteSerializationOptions(), wantToLog);
    			//Logger::logDev("Send Fast Update " + entity.getName() + " to peer " + toString(static_cast<int>(peerId)) + " (" + toString(outboundDataSize) + " B)");

    			if (wantToLog) {
	        		Logger::logInfo("  - send fast serialize msg, " + toString(outboundDataSize) + " bytes");
    			}

    			Bytes bytes(outboundDataSize);
    			memcpy(bytes.data(), fastUpdateOutboundData.data(), outboundDataSize);

    			send(EntityNetworkMessageUpdate(remote.networkId, std::move(bytes), true, remote.hasAuthorityOnly, sessionTimestamp));

    			// Entity is (still) changing, refresh wait timer.
    			if (remote.waitAfterFrameModified > 0) {
    				remote.waitAfterFrameModified = WAIT_UNTIL_DORMANT_AFTER_FRAME_MODIFIED;
    			}

    			++stats.nUpdated;
//				Logger::logDev("Sending " + entity.getName() + " " + entity.getEntityId().toDetailedString(), true);

#if defined(DEV_BUILD) && defined(_WIN32)
    			// Dev build only: similar to above, but the other way around:
    			// requiresEntityFrameModified is set, nobody called setLastFrameModified(), but the entity has been
    			// modified. Peers would miss changes in release builds.
    			if (checkExpectNoUpdate && expectNoUpdate) {
    				Logger::logError("Network entity " + entity.getName() + " has been modified, and requiresEntityFrameModified is set, but no update was signaled", true);
    			}
    			if (checkExpectSameHash && foundSameHash) {
    				Logger::logError("Network entity " + entity.getName() + " has been modified, but fast hash check didn't detect the change", true);
    			}
#endif
    		}

    		if (modifiedInStructure) {
    			canFastUpdate = false;
    			// Wipe the existing journal
    			remote.fastUpdateJournal.clear();
	    		//Logger::logDev("Network entity " + entity.getName() + " has been modified in structure, fall back using slow path");
    			if (checkExpectSameHash && foundSameHash) {
    				Logger::logError("Network entity " + entity.getName() + " has been modified in structure, but fast hash check didn't detect the change", true);
    			}
    		}

    		if (checkExpectSameHash && !modified && !modifiedInStructure && !foundSameHash /*&& entity.getName().contains("lava")*/) {
    			Logger::logError("Network entity " + entity.getName() + " has NOT been modified, but fast hash check found a change " + toString(prevHashRemoveMe, 16));//, true);
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
        auto newData = parentSession->getFactory().serializeEntity(entity, parentSession->getEntitySerializationOptions());
        auto retriever = DataInterpolatorSetRetriever(entity, true);
        auto options = parentSession->getEntityDeltaOptions();
        options.interpolatorSet = &retriever;
        auto deltaData = EntityDataDelta(remote.data, newData, options);

        if (deltaData.hasChange()) {
            remote.data = std::move(newData);

            auto bytes = Serializer::toBytes(deltaData, parentSession->getByteSerializationOptions());
            //Logger::logDev("Send Update " + entity.getName() + " to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B):\n" + deltaData.toYAML() + "\n");
            //Logger::logDev("Send Update " + entity.getName() + " to peer " + toString(static_cast<int>(peerId)) + " (" + toString(bytes.size()) + " B)");

        	if (wantToLog) {
        		Logger::logInfo("  - send EntityDataDelta, " + toString(bytes.size()) + " bytes");
        	}

            send(EntityNetworkMessageUpdate(remote.networkId, std::move(bytes), false, remote.hasAuthorityOnly, sessionTimestamp));

        	// Entity is (still) changing, refresh wait timer.
        	if (remote.waitAfterFrameModified > 0) {
        		remote.waitAfterFrameModified = WAIT_UNTIL_DORMANT_AFTER_FRAME_MODIFIED;
        	}

			++stats.nUpdated;
        }

#if USE_FAST_NETWORK_COMPONENT_UPDATES
        // Binary serialization to (re-)build the update journal.
        if (fastSerializer.serializeEntityUpdate(entity, parentSession->getByteSerializationOptions())) {
	        fastSerializer.processEntityUpdateChanges(remote.fastUpdateJournal, false);
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
		// Don't want (and not allowed) to destroy this here.
		Logger::logError("Attempt to destroy temporary outbound entity");
		return;
	}

	const auto tempInboundIter = std_ex::find_if(tempInboundEntities, [&](const auto& kv) {
		return kv.second.worldId == entityId;
	});

	if (tempInboundIter != tempInboundEntities.end()) {
		// This "usually" happens, on the host, if an entity got destroyed before we got back authority.
		// In scripts, network locks (with authority) are only released on exit, but script nodes or
		// messages that trigger entity destruction in code can happen before that.
		Logger::logWarning("Attempt to destroy outbound entity for " + toString(entityId.value & 0xffffffff) + ", but do not have authority");
		return;
	}

	allocatedOutboundIds.erase(remote.networkId);

	send(EntityNetworkMessageDestroy(remote.networkId));

	//if (const auto entity = parentSession->getWorld().tryGetEntity(entityId); entity.isValid()) {
	//	Logger::logDev("Send Destroy: " + entity.getName() + " (" + entity.getInstanceUUID() +
	//		") with EntityNetworkId (" + remote.networkId +
	//		") to peer " + toString(static_cast<int>(peerId)));
	//} else {
	//	Logger::logDev("Send Destroy: entity does not exist anymore, but EntityNetworkId (" +
	//		toString(remote.networkId) + ") to peer " + toString(static_cast<int>(peerId)));
	//}
}

void EntityNetworkRemotePeer::sendKeepAlive()
{
	send(EntityNetworkMessageKeepAlive());
}

void EntityNetworkRemotePeer::send(EntityNetworkMessage message)
{
	parentSession->sendToPeer(std::move(message), peerId);
	timeSinceSend = 0;
}

void EntityNetworkRemotePeer::receiveCreateEntity(const EntityNetworkMessageCreate& msg)
{
	if (const auto iter = inboundEntities.find(msg.entityId); iter != inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " already exists from peer " + toString(static_cast<int>(peerId)));
		return;
	}

	if (msg.assignNetworkIdOnly) {
		receiveAssignEntity(msg);
		return;
	}

	auto delta = Deserializer::fromBytes<EntityDataDelta>(msg.bytes, parentSession->getByteSerializationOptions());
	if (!delta.getInstanceUUID()) {
		if (delta.getPrefab()) {
			Logger::logWarning("Unable to instantiate network entity, no instance UUID, prefab: " + delta.getPrefab());
		} else {
			Logger::logWarning("Unable to instantiate network entity, no instance UUID");
		}
		return;
	}

	const auto targetParentUUID = delta.getParentUUID().value_or(UUID());
	if (targetParentUUID.isValid()) {
		const auto parentEntity = parentSession->getWorld().findEntity(targetParentUUID);
		if (!parentEntity) {
			//Logger::logError("Can't instantiate network entity " + toString(msg.entityId) + " from prefab \"" + delta.getPrefab() + "\" - parent " + toString(targetParentUUID) + " not found");
			const auto instanceUUID = delta.getInstanceUUID().value_or(UUID());
			pendingEntities[msg.entityId] = PendingEntity{ msg.entityId, {instanceUUID, targetParentUUID}, std::move(delta), msg.worldPartition, {} };
			return;
		} else if (!parentEntity->isValid() || !parentEntity->isAlive()) {
			Logger::logError("Can't instantiate network entity " + toString(msg.entityId) + " from prefab \"" + delta.getPrefabUUID() + "\" - parent " + toString(targetParentUUID) + " found but in unexpected state");
			return;
		}
	}

	createRemoteEntity(msg.entityId, delta, msg.worldPartition != 0);
}

void EntityNetworkRemotePeer::receiveAssignEntity(const EntityNetworkMessageCreate& msg)
{
	const auto info = Deserializer::fromBytes<EntityNetworkInstanceInfo>(msg.bytes, parentSession->getByteSerializationOptions());
	auto entity = parentSession->getWorld().findEntity(info.instanceUUID);

	if (!entity) {
		pendingEntities[msg.entityId] = PendingEntity{ msg.entityId, info, {}, msg.worldPartition, {} };
		Logger::logDev("Store pending network entity " + toString(msg.entityId) + " for world entity - entity not found");
		return;
	}

	if (const auto parentEntity = parentSession->getWorld().findEntity(info.parentUUID)) {
		entity->setParent(parentEntity.value());
	}

	assignRemoteEntity(msg.entityId, entity.value());
}

void EntityNetworkRemotePeer::receiveUpdateEntity(const EntityNetworkMessageUpdate& msg)
{
	if (msg.forAuthorityOnly) {
		// An update message for entity with changed authority.
		if (const auto iter = tempInboundEntities.find(msg.entityId); iter != tempInboundEntities.end()) {
			auto& remote = iter->second;
			const auto entity = parentSession->getWorld().tryGetEntity(remote.worldId);
			if (entity.isValid()) {
				updateRemoteEntity(remote, entity, msg);
			} else {
				Logger::logWarning("No entity found " + toString(remote.worldId.value & 0xffffffff) + " for inbound message with authority-only, dropping message");
			}
		} else {
			// NB: it's possible to receive updates after authority/lock has been released already
			Logger::logDev("No temporary inbound entity with network id " + toString(static_cast<int>(msg.entityId)) + " found", true);
		}
		return;
	}

	if (const auto iter = pendingEntities.find(msg.entityId); iter != pendingEntities.end()) {
		updatePendingEntity(iter->second, msg);
		return;
	}

	const auto iter = inboundEntities.find(msg.entityId);
	if (iter == inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " not found from peer " + toString(static_cast<int>(peerId)), true);
		return;
	}
	auto& remote = iter->second;

	auto entity = parentSession->getWorld().tryGetEntity(remote.worldId);
	if (!entity.isValid()) {
		Logger::logWarning("Entity with network id (" + toString(static_cast<int>(msg.entityId)) + ") and EntityId (" + toString(remote.worldId.value & 0xffffffff) + ") not alive in the world from peer " + toString(static_cast<int>(peerId)));
		if (!msg.fastSerialize) {
			const auto delta = Deserializer::fromBytes<EntityDataDelta>(msg.bytes, parentSession->getByteSerializationOptions());
			Logger::logWarning("Caused by trying to update entity:\n" + delta.toYAML());
		}
		return;
	}

	updateRemoteEntity(remote, entity, msg);
}

void EntityNetworkRemotePeer::receiveDestroyEntity(const EntityNetworkMessageDestroy& msg)
{
	if (const auto iter = pendingEntities.find(msg.entityId); iter != pendingEntities.end()) {
		pendingEntities.erase(iter);
		return;
	}

	const auto iter = inboundEntities.find(msg.entityId);
	if (iter == inboundEntities.end()) {
		Logger::logWarning("Entity with network id " + toString(static_cast<int>(msg.entityId)) + " not found from peer " + toString(static_cast<int>(peerId)));
		return;
	}
	const auto& remote = iter->second;

	//if (const auto entityRef = parentSession->getWorld().tryGetEntity(remote.worldId); entityRef.isValid()) {
	//	Logger::logDev("Destroying from network: " + entityRef.getName() + " UUID " + toString(entityRef.getInstanceUUID()) + " NetworkEntityId (" + toString(static_cast<int>(msg.entityId)) + ") and EntityId(" + toString(remote.worldId) + ")");
	//} else {
	//	Logger::logDev("Destroying from network, but entity not found: NetworkEntityId (" + toString(static_cast<int>(msg.entityId)) + ") and EntityId(" + toString(remote.worldId) + ")");
	//}

	destroyRemoteEntity(remote);

	inboundEntities.erase(msg.entityId);
}

EntityRef EntityNetworkRemotePeer::createRemoteEntity(EntityNetworkId id, const EntityDataDelta& delta, bool allowExistingLookup)
{
	const auto debugInfo = EntityFactory::DebugInfo("NetworkEntity " + toString(id), EntityLoadContextType::Network);

	// Generate EntityData from prefab, with delta applied.
	auto [entityData, prefab, prefabUUID] = parentSession->getFactory().prefabDeltaToEntityData(delta, *delta.getInstanceUUID(), debugInfo);
	if (!entityData) {
		Logger::logError("Unable to instantiate network entity " + toString(static_cast<int>(id)));
		return {};
	}

	EntityRef entity;
	bool appliedOnExistingEntity = false;

	const auto existingEntity = parentSession->getFactory().getWorld().findEntity(entityData->getInstanceUUID());
	if (existingEntity && existingEntity->isValid()) {
		if (!allowExistingLookup) {
			// NB: this may be unlikely but possible to happen, if there's some network hickup.
			Logger::logWarning("Can't create remote entity " + entityData->getName() + ", " +
				entityData->getInstanceUUID().toString() + ", already exists");
			return {};
		}

		entity = existingEntity.value();
		appliedOnExistingEntity = true;
	}

	if (!appliedOnExistingEntity) {
		// NB: This used to just call EntityFactory::loadEntityDelta(), but this doesn't properly
		// handle some cases we need to cover here. So instead, let's create a new entity from
		// prefab, then *update* immediately by applying the delta.
		if (prefab) {
			// Instantiate entity from prefab first.
			EntityData prefabData(entityData->getInstanceUUID());
			prefabData.setPrefab(prefab->getAssetId());
			entity = parentSession->getFactory().createEntity(prefabData, EntitySerialization::makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::Network));

			// Apply the update.
			parentSession->getFactory().updateEntity(entity, delta, EntitySerialization::makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::Network));
		} else {
			// No prefab in delta, try instantiating from delta directly.
			entity = parentSession->getFactory().createEntity(*entityData, EntitySerialization::makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::Network));
		}
	} else {
		// Apply update on the existing entity.
		parentSession->getFactory().updateEntity(entity, delta, EntitySerialization::makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::Network));
	}

	if (const UUID& parentUUID = entityData->getParentUUID(); parentUUID.isValid()) {
		if (auto parentEntity = parentSession->getWorld().findEntity(parentUUID); parentEntity) {
			if (parentEntity != entity.tryGetParent()) {
				entity.setParent(parentEntity.value());
			}
		} else {
			Logger::logError("Parent " + toString(parentUUID) + " not found for network entity \"" + entity.getName() + "\" (" + entity.getInstanceUUID() + ")");
		}
	}

	//Logger::logDev("Created entity " + entity.getName() + " with EntityNetworkId (" + toString(msg.entityId) + ") and EntityId (" + toString(entity.getEntityId()) + ") from network:\n\n" + EntityData(delta).toYAML());
	//Logger::logDev("Created entity " + entity.getName() + " with EntityNetworkId (" + toString(msg.entityId) + ") and EntityId (" + toString(entity.getEntityId()) + ") Instance UUID " + toString(entity.getInstanceUUID()));
	//if (entity.getParent().isValid()) {
	//	Logger::logDev("with parent " + entity.getParent().getName());
	//}

	//Logger::logDev("Assigning network id: " + toString(static_cast<int>(msg.entityId)) + " to new entity " + entity.getName());

	InboundEntity remote;
	remote.data = std::move(*entityData);
	remote.worldId = entity.getEntityId();
	remote.appliedOnExistingEntity = appliedOnExistingEntity;
	remote.debugName = entity.getName() + "|" + entity.getPrefabAssetId();
	inboundEntities[id] = std::move(remote);

	auto& interpolatorSet = entity.setupNetwork(peerId);
	parentSession->onRemoteEntityCreated(entity, peerId);
	parentSession->requestSetupInterpolators(interpolatorSet, entity, true);

	auto& byteDataInterpolatorSet = entity.getComponent<NetworkComponent>(true).byteDataInterpolatorSet;
	parentSession->requestSetupByteDataInterpolators(byteDataInterpolatorSet, entity);

	return entity;
}

void EntityNetworkRemotePeer::assignRemoteEntity(EntityNetworkId id, EntityRef entity)
{
	HalleyAssertDev(entity.hasParent());

	//Logger::logDev("Assigning network id: " + toString(static_cast<int>(id)) + " to existing entity " + entity->getName());

	InboundEntity remote;
	// Construct entity data from local entity, since the host doesn't send any delta in this case.
	remote.data = parentSession->getFactory().serializeEntity(entity, parentSession->getEntitySerializationOptions());
	remote.worldId = entity.getEntityId();
	remote.appliedOnExistingEntity = true;
	remote.debugName = entity.getName();
	inboundEntities[id] = std::move(remote);

	auto& interpolatorSet = entity.setupNetwork(peerId);
	parentSession->onRemoteEntityCreated(entity, peerId);
	parentSession->requestSetupInterpolators(interpolatorSet, entity, true);

	auto& byteDataInterpolatorSet = entity.getComponent<NetworkComponent>().byteDataInterpolatorSet;
	parentSession->requestSetupByteDataInterpolators(byteDataInterpolatorSet, entity);
}

void EntityNetworkRemotePeer::updateRemoteEntity(InboundEntity& inboundEntity, EntityRef entity, const EntityNetworkMessageUpdate& msg)
{
	int32_t timestamp = msg.timestamp; // Time the peer sent this msg, using its approximation of our own local network time.

	if (msg.fastSerialize) {
        //Logger::logDev("Receive Fast Update " + entity.getName() + " (" + toString(msg.bytes.size()) + " B)");

        try {
	    	fastSerializer.setSession(parentSession);
            auto result = fastSerializer.deserializeEntityUpdate(entity, msg.bytes, parentSession->getByteSerializationOptions());
        	if (result.position) {
        		updateRemoteEntityPosition(inboundEntity, result.position.value(), timestamp);
        	} else {
        		inboundEntity.positionUpdates.clear();
        	}
        } catch (const std::exception& e) {
            Logger::logError("Exception while processing update entity from network");
            Logger::logException(e);
        }
    } else {
    	if (inboundEntity.forChangedAuthorityOnly) {
            Logger::logError("Full network updates unsupported for temporary inbound entities");
    		return;
    	}

        const auto delta = Deserializer::fromBytes<EntityDataDelta>(msg.bytes, parentSession->getByteSerializationOptions());

        auto retriever = DataInterpolatorSetRetriever(entity, false);
        //Logger::logDev("Receive Update " + entity.getName() + " (" + toString(msg.bytes.size()) + " B)");
        //Logger::logDev("Updating entity " + entity.getName() + ":\n" + delta.toYAML());

        try {
            parentSession->getFactory().updateEntity(entity, delta,
                                              EntitySerialization::makeMask(EntitySerialization::Type::Network),
                                              nullptr, &retriever);
        } catch (const std::exception &e) {
            Logger::logError("Exception while processing update entity from network:\n" + delta.toYAML());
            Logger::logException(e);
        }
        inboundEntity.data.applyDelta(delta);
    }
}

void EntityNetworkRemotePeer::destroyRemoteEntity(const InboundEntity& inboundEntity)
{
	auto entity = parentSession->getWorld().tryGetEntity(inboundEntity.worldId);
	if (entity.isValid()) {
		bool shouldBeDeleted = !inboundEntity.appliedOnExistingEntity // don't delete world entities (loaded from chunk)
			&& !inboundEntity.forChangedAuthorityOnly; // local peer owns this, let nobody tell us what to delete

		entity.setFromNetwork(false);

		if (shouldBeDeleted) {
			parentSession->getWorld().destroyEntity(entity);
		}
	} else if (parentSession->isHost()) {
		// NB: This can be very legit for peers.
		// Updates can arrive after a chunk has been unloaded.
		Logger::logWarning("Network entity has gone missing: " + inboundEntity.debugName);
	}
}

void EntityNetworkRemotePeer::trySpawningPendingEntities()
{
	Vector<EntityNetworkId> toRemove;
	for (auto& [id, data]: pendingEntities) {
		if (!data.data) {
			if (auto entity = parentSession->getWorld().findEntity(data.instanceInfo.instanceUUID)) {
				// Keeps this in pending state until both entity and parent are available.
				// TODO: this doesn't check if it's the right parent though
				if (!entity->hasParent()) {
					if (auto parentEntity = parentSession->getWorld().findEntity(data.instanceInfo.parentUUID); parentEntity) {
						entity->setParent(parentEntity.value());
					}
				} else {
					assignPendingEntity(data, entity.value());
					toRemove += id;
				}
			}
		} else {
			if (const auto parentEntity = parentSession->getWorld().findEntity(data.data->getParentUUID().value_or(UUID()))) {
				if (parentEntity) {
					createPendingEntity(data);
					toRemove += id;
				}
			}
		}
	}

	if (!toRemove.empty()) {
		std_ex::erase_if_key(pendingEntities, [&] (EntityNetworkId id) {
			return toRemove.contains(id);
		});
		//Logger::logDev("created " + toString(toRemove.size()) + " pending entities, " + toString(pendingEntities.size()) + " left");
	}
}

void EntityNetworkRemotePeer::createPendingEntity(const PendingEntity& pendingData)
{
	HalleyAssertDev(pendingData.data);

	const auto entity = createRemoteEntity(pendingData.id, pendingData.data.value(), pendingData.worldPartition != 0);

	if (!entity.isValid()) {
		Logger::logError("Error when creating pending entity - entity creation failed");
		return;
	}

	if (const auto iter = inboundEntities.find(pendingData.id); iter != inboundEntities.end()) {
		auto& remote = iter->second;
		for (const auto& update: pendingData.updates) {
			updateRemoteEntity(remote, entity, update);
		}
	} else {
		Logger::logError("Error when creating pending entity - inbound entity data not found");
	}
}

void EntityNetworkRemotePeer::assignPendingEntity(const PendingEntity& pendingData, EntityRef entity)
{
	HalleyAssertDev(!pendingData.data);

	assignRemoteEntity(pendingData.id, entity);

	if (const auto iter = inboundEntities.find(pendingData.id); iter != inboundEntities.end()) {
		auto& remote = iter->second;
		for (const auto& update : pendingData.updates) {
			updateRemoteEntity(remote, entity, update);
		}
	} else {
		Logger::logError("Error when assigning pending entity - inbound entity data not found");
	}
}

void EntityNetworkRemotePeer::updatePendingEntity(PendingEntity& entity, const EntityNetworkMessageUpdate& msg)
{
	entity.updates += msg;
}

bool EntityNetworkRemotePeer::isRemoteReady() const
{
	auto& sharedData = parentSession->getSession().getClientSharedData<EntityClientSharedData>(peerId);
	return sharedData.viewRect.has_value();
}

void EntityNetworkRemotePeer::onFirstDataBatchSent()
{
	if (parentSession->getSession().getType() == NetworkSessionType::Host) {
		send(EntityNetworkMessage(EntityNetworkMessageReadyToStart()));
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

std::pair<bool, std::optional<EntityNetworkId>> EntityNetworkRemotePeer::prepareChangeEntityAuthority(EntityId entityId, NetworkSession::PeerId myPeerId,
	NetworkSession::PeerId ownerId, const std::optional<NetworkSession::PeerId>& authorityId, const std::optional<EntityNetworkId>& assignNetworkId)
{
	std::pair<bool, std::optional<EntityNetworkId>> result = {true, {}};

	if (authorityId.has_value()) {
		if (myPeerId == ownerId) {
			// I lose authority. Create a temporary inbound entity.
			HalleyAssertDebug(!assignNetworkId.has_value());

			// If somebody wants to grab authority of a child entity, it's very likely that there
			// is no outbound entity just yet. So let's create one, and return its network ID
			// assigned up the call chain.
			if (!outboundEntities.contains(entityId)) {
				const auto entityRef = parentSession->getWorld().getEntity(entityId);
				if (entityRef.isValid()) {
					const auto networkComponent = entityRef.tryGetComponent<NetworkComponent>();
					if (networkComponent) {
						OutboundEntity outbound = {};
						outbound.alive = true;
						outbound.forChildEntityTemporaryOnly = true;
						outbound.networkId = assignId();

						//Logger::logDev("Assign temporary outbound entity for " + entityRef.getName() +
						//	", " + toString(entityRef.getEntityId().value & 0xffffffff) + ", network ID " + toString(outbound.networkId));

						outboundEntities[entityId] = std::move(outbound);
					}
				}
			}

			// There should (now) be some outbound entity available.
			if (outboundEntities.contains(entityId)) {
				const auto &oe = outboundEntities[entityId];

				// Always return assigned network ID to caller.
				result.second = oe.networkId;

				InboundEntity remote;
				remote.worldId = entityId;
				remote.debugName = "[temp authority]";
				remote.forChangedAuthorityOnly = true;

				//Logger::logDev("Create temporary inbound entity for " +
				//	toString(entityId.value & 0xffffffff) + ", network ID " + toString(oe.networkId));

				if (!tempInboundEntities.contains(oe.networkId)) {
					tempInboundEntities[oe.networkId] = std::move(remote);
				} else {
					Logger::logWarning("Entity with network id " + toString(oe.networkId) + " already has a temporary inbound entity");
				}
			} else {
				Logger::logWarning("No outbound entity " + toString(entityId.value & 0xffffffff) + " found to create temporary inbound entity from", true);
				return {false, {}};
			}
		} else if (myPeerId == authorityId) {
			// I'm taking authority. Create a temporary outbound entity.
			if (!assignNetworkId.has_value()) {
				Logger::logError("Expected network ID to be passed by caller", true);
				return {false, {}};
			}

			OutboundEntity outbound = {};
			outbound.alive = true;
			outbound.hasAuthorityOnly = true;
			outbound.networkId = assignNetworkId.value();

			// Search for existing inbound entity.
			auto inboundIter = std_ex::find_if(inboundEntities, [&](const auto& kv) {
				return kv.second.worldId == entityId;
			});

			// Just like above: if this is a child entity, it's possible that there is no inbound
			// entity. In this case, we flag it and use the network ID sent by the caller.
			if (inboundIter == inboundEntities.end()) {
				outbound.forChildEntityTemporaryOnly = true;
			} else {
				if (inboundIter->first != assignNetworkId.value()) {
					Logger::logError("Inbound entity exists, but assigned network ID does not match");
				}
			}

			//Logger::logDev("Create temporary outbound entity for " +
			//	toString(entityId.value & 0xffffffff) + ", network ID " + toString(outbound.networkId));

			if (!outboundEntities.contains(entityId)) {
				outboundEntities[entityId] = std::move(outbound);
			} else {
				Logger::logWarning("Entity id " + toString(entityId.value & 0xffffffff) + " already has an outbound entity");
			}
		}
	} else {
		// Authority has been given back.

		// This shouldn't be called with some ID to assign.
		HalleyAssertDebug(!assignNetworkId.has_value());

		// Remove the temporary inbound entity:
		// - for the owner, this is the one created for receiving updates
		// - for the peer releasing authority, this might be one ... TODO we don't actually need?
		const size_t found = std_ex::erase_if_value(tempInboundEntities, [&](const auto& value) {
			if (value.worldId == entityId) {
				HalleyAssertDev(value.forChangedAuthorityOnly);
				return true;
			}
			return false;
		});
		if (found > 1) {
			Logger::logWarning(toString(found) + " temporary inbound entities deleted, should be 1");
		}

		if (outboundEntities.contains(entityId)) {
			// Remove any temporary outbound entity:
			// - for the owner, this might be one created for child entities
			// - for the peer releasing authority, it's the one created to send updates with
			const auto& oe = outboundEntities[entityId];
			if (oe.hasAuthorityOnly || oe.forChildEntityTemporaryOnly) {
				//auto entityRef = parentSession->getWorld().tryGetEntity(entityId);
				//Logger::logDev("Remove temporary outbound entity for " + entityRef.getName() + ", ID " +
				//	toString(entityId.value & 0xffffffff) + ", network ID " + toString(outboundEntities[entityId].networkId));

				outboundEntities.erase(entityId);
			}
		} else {
			Logger::logWarning("No (temporary) outbound entity found for " + toString(entityId.value & 0xffffffff));
		}
	}

	return result;
}

void EntityNetworkRemotePeer::updateRemoteEntityPosition(InboundEntity& inboundEntity, const Vector2f& position, int32_t timestamp)
{
	auto& entries = inboundEntity.positionUpdates;

	if (entries.empty() || entries.back().second < timestamp) {
		// This "should" be the default: latest position update, just append.
		entries.emplace_back(position, timestamp);
	} else {
		// Insert, to keep entries sorted by timestamp.
		const size_t sz = entries.size();
		size_t idx = 0;

		while (idx < sz) {
			const size_t t = entries[idx].second;

			if (t == timestamp) {
				// Same timestamp: only update position.
				entries[idx].first = position;
				break;
			}

			if (t > timestamp) {
				// Found position to insert.
				entries.insert(entries.begin() + static_cast<ptrdiff_t>(idx), {position, timestamp});
				break;
			}

			idx++;
		}
	}
}

void EntityNetworkRemotePeer::interpolateRemoteEntityPosition(InboundEntity& inboundEntity, int32_t now, int32_t latency)
{
	auto& entries = inboundEntity.positionUpdates;

	size_t sz = entries.size();
	size_t idx = 0;

	// Remove entries with elapsed timestamps.
	while (idx < sz) {
		if (entries[idx].second >= now - latency * 3) {
			break;
		}
		idx++;
	}

	if (idx != 0) {
		entries.erase(entries.begin(), entries.begin() + static_cast<ptrdiff_t>(idx));
		sz -= idx;
	}

	auto entity = parentSession->getWorld().tryGetEntity(inboundEntity.worldId);

	if (!entity.isValid()) {
		// Can happen if entity was just destroyed on local peer.
		return;
	}

	auto transform = entity.tryGetComponent<Transform2DComponent>();
	if (transform == nullptr) {
		return;
	}

	auto pos = transform->getLocalPosition();

	// If there are no more pending position updates, push local position at current timestamp.

	if (sz == 0) {
		entries.emplace_back(pos, now - latency);
		return;
	}

	// Assuming the entries are sorted (which is done in updateRemoteEntityPosition()),
	// look for the entries covering the current timestamp.

	idx = 0;
	while (idx + 1 < sz) {
		if (entries.at(idx).second > now - latency) {
			break;
		}
		idx++;
	}

	if (idx + 1 == sz) {
		// At last position, or there's only one entry.
		pos = entries.back().first;
	} else {
		// Actual interpolation.
		const auto& e0 = entries.at(idx);
		const auto& e1 = entries.at(idx + 1);

		float t = static_cast<float>((now - latency) - e0.second) / static_cast<float>(e1.second - e0.second);

		pos = (1.0f - t) * e0.first + t * e1.first;
	}

	HalleyAssertDev(pos.isValid());
	transform->setLocalPosition(pos);
}

void EntityNetworkRemotePeer::interpolateRemoteEntityPositions(Time dt)
{
	if (dt < 0.001) return; // called more than once per frame

	const auto& session = parentSession->getSession();

	int32_t now = session.getLocalSessionTimeMs();
	int32_t latency = session.getLatency(session.getIndexOfRemotePeer(peerId));

	for (auto& [_, inboundEntity] : inboundEntities) {
		if (outboundEntities.contains(inboundEntity.worldId)) {
			// TODO: this could be a flag in inboundEntity
			HalleyAssertDebug(outboundEntities[inboundEntity.worldId].hasAuthorityOnly);
			continue;
		}
		interpolateRemoteEntityPosition(inboundEntity, now, latency);
	}

	for (auto& [_, inboundEntity] : tempInboundEntities) {
		interpolateRemoteEntityPosition(inboundEntity, now, latency);
	}
}
