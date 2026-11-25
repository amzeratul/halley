#include <systems/network_lock_system.h>

using namespace Halley;

class INetworkLockSystem {
public:
	virtual ~INetworkLockSystem() = default;
	virtual void lockRelease(EntityId playerId, EntityId targetId) = 0;
};

class NetworkLock : public INetworkLock {
public:
	NetworkLock(INetworkLockSystem& parent, EntityId playerId, EntityId targetId)
		: parent(parent)
		, playerId(playerId)
		, targetId(targetId)
	{}

	~NetworkLock() override
	{
		parent.lockRelease(playerId, targetId);
	}

private:
	INetworkLockSystem& parent;
	EntityId playerId;
	EntityId targetId;
};

class NetworkLockSystem final : public NetworkLockSystemBase<NetworkLockSystem>, INetworkLockSystemInterface, INetworkLockSystem, NetworkSession::IListener
{
public:
	void init()
	{
		getWorld().setInterface(static_cast<INetworkLockSystemInterface*>(this));

		initSession(getSessionService().getSession());
		sessionChangedToken = getSessionService().addSessionChangeCallback([this] (SessionService::ChangeData data) {
			deInitSession(*data.oldSession);
			initSession(*data.newSession);
		});
	}

	void deInit() override
	{
		deInitSession(getSessionService().getSession());
	}

	void initSession(Session& session)
	{
		if (const auto networkSession = session.getEntityNetworkSession()) {
			networkSession->getSession().addListener(this);
		}
	}

	void deInitSession(Session& session)
	{
		if (const auto networkSession = session.getEntityNetworkSession()) {
			networkSession->getSession().removeListener(this);
		}
	}

	void update(Time t)
	{
		checkStaleLocks({});
		hasRemoteLockCache = false;
	}

	[[nodiscard]] LockStatus getLockStatus(EntityId targetId) const override
	{
		return getLockStatus(targetId, getMyPeerId());
	}

	[[nodiscard]] LockStatus getLockStatus(EntityId targetId, uint8_t myPeerId) const
	{
		targetId = resolveLockTarget(targetId, true);

		if (auto localStatus = getLocalLockStatus(targetId, myPeerId); localStatus != LockStatus::Unlocked) {
			return localStatus;
		}

		return getRemoteLockStatus(targetId, myPeerId);
	}

	[[nodiscard]] LockStatus makeLockStatus(bool mine, NetworkLockType type) const
	{
		const bool shared = type == NetworkLockType::Shared;
		if (mine) {
			return shared ? LockStatus::SharedAcquiredByMe : LockStatus::UniqueAcquiredByMe;
		} else {
			return shared ? LockStatus::SharedAcquiredByOther : LockStatus::UniqueAcquiredByOther;
		}
	}

	[[nodiscard]] LockStatus getLocalLockStatus(EntityId targetId, uint8_t myPeerId) const
	{
		if (const auto iter = myLocks.find(targetId); iter != myLocks.end()) {
			return makeLockStatus(myPeerId == getMyPeerId(), iter->second.type);
		}
		return LockStatus::Unlocked;
	}

	[[nodiscard]] LockStatus getRemoteLockStatus(const NetworkFamily* e, uint8_t myPeerId, bool includeMine) const
	{
		if (e->network.authorityId) {
			return e->network.authorityId == myPeerId ? (includeMine ? LockStatus::UniqueAcquiredByMe : LockStatus::Unlocked) : LockStatus::UniqueAcquiredByOther;
		}

		bool otherShared = false;
		for (auto& lock: e->network.locks) {
			const bool mine = lock.peerId == myPeerId;
			if (mine && !includeMine) {
		        continue; // my own locks on relatives never affect me
		    }

			const auto status = makeLockStatus(mine, lock.type);
			if (status != LockStatus::SharedAcquiredByOther) {
				return status;
			}
			otherShared = true;
		}
		return otherShared ? LockStatus::SharedAcquiredByOther : LockStatus::Unlocked;
	}

	[[nodiscard]] LockStatus getRemoteLockStatus(EntityId targetId, uint8_t myPeerId) const
	{
		const NetworkFamily* e = tryFindNetworkRoot(targetId);
		if (!e) {
			auto entity = getWorld().tryGetEntity(targetId);
			//Logger::logWarning("Trying to check lock status of non-network entity " + toString(targetId) + (entity.isValid() ? " (" + entity.getName() + ")" : ""), true);
			return LockStatus::Unlocked;
		}

		const auto ns = collectNetworkAncestorsAndDescendantsCached(getWorld().tryGetEntity(e->entityId));

		LockStatus result = LockStatus::Unlocked;
		for (const auto* n: ns) {
			result = std::max(result, getRemoteLockStatus(n, myPeerId, n == e));
		}
		return result;
	}

	[[nodiscard]] bool isLockedByOrAvailableTo(EntityId playerId, EntityId targetId, NetworkLockType lockType) const override
	{
		uint8_t peerId = 0;
		if (const NetworkFamily* locker = networkFamily.tryFind(playerId)) {
			peerId = locker->network.ownerId.value_or(0);
		} else {
			peerId = getMyPeerId();
		}
		const auto status = getLockStatus(targetId, peerId);
		return status == LockStatus::Unlocked || status == LockStatus::UniqueAcquiredByMe || (lockType == NetworkLockType::Shared && (status == LockStatus::SharedAcquiredByOther || status == LockStatus::SharedAcquiredByMe));
	}

	enum class LockAcquireResult {
		Acquired,
		Refused,
		NotHeld
	};

	[[nodiscard]] std::pair<LockAcquireResult, NetworkLockHandle> tryAcquireLocal(EntityId playerId, EntityId targetId, NetworkLockType lockType)
	{
		// targetId should have gone through resolveLockTarget already
		if (!targetId.isValid()) {
			// Invalid entity
			return { LockAcquireResult::Refused, {} };
		}

		if (const auto iter = myLocks.find(targetId); iter != myLocks.end()) {
			auto& lock = iter->second;
			if (lock.playerId.isValid()) {
				// Checks the refCount - can be zero on a peer if the lock has been lifted, but the
				// message sent to the host is about to be delivered still...
				if (lock.refCount > 0) {
					// Locked by some local entity
					if (lock.playerId == playerId && lockType <= lock.type) {
						// Already locked by this player.
						// Increment lock count!
						lock.refCount++;

						return { LockAcquireResult::Acquired, std::make_shared<NetworkLock>(static_cast<INetworkLockSystem&>(*this), playerId, targetId) };
					} else {
						// Locked by someone else
						return { LockAcquireResult::Refused, {} };
					}
				}
			} else if (lock.refCount == 0) {
				// Preliminary lock, waiting for result from host. Block any follow-up attempt.
				Logger::logWarning("Tried to aquire lock, but previous request still pending");
				return { LockAcquireResult::Refused, {} };
			}
		}

		return { LockAcquireResult::NotHeld, {} };
	}

	NetworkLockHandle lockReacquireLocal(EntityId playerId, EntityId targetId, NetworkLockType lockType) override
	{
		return tryAcquireLocal(playerId, resolveLockTarget(targetId, false), lockType).second;
	}

	Future<NetworkLockHandle> lockAcquire(EntityId playerId, EntityId targetId, NetworkLockType lockType) override
	{
		targetId = resolveLockTarget(targetId, false);

		if (auto [result, local] = tryAcquireLocal(playerId, targetId, lockType); result != LockAcquireResult::NotHeld) {
			// Either succeeded or was refused
			return Future<NetworkLockHandle>::makeImmediate(std::move(local));
		}

		// Not locked locally, try to lock
		return doLockAcquireForMe(targetId, lockType).then([=, this] (bool success) -> NetworkLockHandle
		{
			if (success) {
				auto& l = myLocks[targetId];
				l.playerId = playerId;
				l.refCount++;
                l.type = lockType;
				return std::make_shared<NetworkLock>(static_cast<INetworkLockSystem&>(*this), playerId, targetId);
			}
			return {};
		});
	}

	void lockUpdate(EntityId targetId, bool destroyOnUnlock) override
	{
		targetId = resolveLockTarget(targetId, true);
		const auto iter = myLocks.find(targetId);
		if (iter != myLocks.end()) {
			if (iter->second.type == NetworkLockType::Shared) {
				Logger::logError("Attempting to set destroyOnUnlock on shared lock is not supported");
			} else {
				iter->second.destroyOnUnlock = destroyOnUnlock;
			}
		}
	}

	void lockRelease(EntityId playerId, EntityId targetId) override
	{
		if (const auto iter = myLocks.find(targetId); iter != myLocks.end()) {
			if (auto& lock = iter->second; lock.refCount > 0) {
				if (lock.playerId == playerId) {
					lock.refCount--;
					if (lock.refCount == 0) {
						doLockReleaseForMe(targetId, lock.type, lock.destroyOnUnlock);
					}
				} else {
					Logger::logError("Releasing network lock with handle that isn't locking it!");
				}
			} else {
				Logger::logError("Releasing network lock with refCount=0");
			}
		} else {
			Logger::logError("Releasing network lock for entity that isn't locked!");
		}		
	}

	ConfigNode onMessageReceived(NetworkEntityLockSystemMessage msg) override
	{
		if (msg.lockType > static_cast<uint8_t>(NetworkLockType::UniqueWithAuthority)) {
			Logger::logError("Invalid lock type");
			ConfigNode::MapType result;
			result["success"] = false;
			return result;
		}

		const auto lockType = static_cast<NetworkLockType>(msg.lockType);
		const auto [success, networkId] = doEntityLockAction(msg.target, msg.peerId, msg.lock, lockType, msg.destroyOnUnlock);

		ConfigNode::MapType result;
		result["success"] = success;
		if (networkId.has_value()) {
			result["id"] = networkId.value();
		}

		return result;
	}

	void onPeerDisconnected(NetworkSession::PeerId peerId) override
	{
		checkStaleLocks(peerId);
		sendMessage(NetworkPeerDisconnectSystemMessage(peerId));
	}

private:
	struct LocalLock {
		EntityId playerId;
		int refCount = 0;
		bool destroyOnUnlock = false;
		NetworkLockType type = NetworkLockType::Unique;
	};
	HashMap<EntityId, LocalLock> myLocks;
	ListenerSetToken sessionChangedToken;

	mutable Vector<EntityId> remoteLockCache;
	mutable bool hasRemoteLockCache = false;

	void checkStaleLocks(std::optional<NetworkSession::PeerId> otherPeerId)
	{
		if (isHost()) {
			auto predicate = [&](uint8_t peerId) {
				return otherPeerId ? peerId == otherPeerId : !isPeerPresent(peerId);
			};

			for (const auto& e: networkFamily) {
				// If this peer took entity authority, return it to host.
				if (e.network.authorityId && predicate(e.network.authorityId.value())) {
					auto [success, _] = doChangeAuthority(&e, {}, {});
					if (success) {
						Logger::logDev("Stale lock released, peer is gone");
					}
				}
				// Erase any stale locks.
				const auto modified = std_ex::erase_if(e.network.locks, [&] (const NetworkLockInfo& info) { return predicate(info.peerId); });
				if (modified) {
					notifyEntityModified(&e);
				}
			}
		}
	}

	const Vector<EntityId>& getRemoteLockCache() const
	{
		if (!hasRemoteLockCache) {
			updateRemoteLockCache();
		}
		return remoteLockCache;
	}

	void updateRemoteLockCache() const
	{
		remoteLockCache.clear();
		for (const auto& e: networkFamily) {
			if (!e.network.locks.empty() || e.network.authorityId.has_value()) {
				remoteLockCache += e.entityId;
			}
		}
		hasRemoteLockCache = true;
	}

	Future<bool> doLockAcquireForMe(EntityId targetId, NetworkLockType lockType)
	{
		if (isHost()) {
			auto [success, networkId] = doEntityLockAction(targetId, getMyPeerId(), true, lockType, false);
			HalleyAssertDebug(!networkId.has_value());
			return Future<bool>::makeImmediate(success);
		} else {
			auto entityNetworkSession = getSessionService().getMultiplayerSession().getEntityNetworkSession();
			if (!entityNetworkSession || !entityNetworkSession->isConnected()) {
				return Future<bool>::makeImmediate(false);
			}

			if (!myLocks.contains(targetId)) {
				// Store a preliminary lock, assigned to invalid player ID.
				// This is done to ensure that follow-up locking attempts by the same client are
				// blocked until the result is delivered.
				LocalLock& l = myLocks[targetId];
				l.playerId = {};
				l.type = lockType;
			}

			Promise<bool> promise;
			auto future = promise.getFuture();
			sendMessage(NetworkEntityLockSystemMessage(targetId, true, static_cast<uint8_t>(lockType), false, getMyPeerId()), [=, this, promise = std::move(promise)] (ConfigNode result) mutable
			{
				bool success = result["success"].asBool(false);

				std::optional<EntityNetworkId> assignNetworkId;
				if (result.hasKey("id")) {
					assignNetworkId = result["id"].asInt();
				}

                if (success && lockType == NetworkLockType::UniqueWithAuthority) {
                    auto [ok, networkId] = changeAuthority(targetId, getMyPeerId(), assignNetworkId);
                	HalleyAssertDebug(!networkId.has_value());
                	success = ok;
                }

				if (!success && !myLocks[targetId].playerId.isValid()) {
					// On failure, remove any preliminary lock.
					myLocks.erase(targetId);
				}

				promise.setValue(success);
			});
			return future;
		}
	}

	void doLockReleaseForMe(EntityId targetId, NetworkLockType lockType, bool destroyOnUnlock)
	{
		if (lockType == NetworkLockType::Shared && destroyOnUnlock) {
			destroyOnUnlock = false;
			Logger::logError("Attempting to release shared lock with destroyOnUnlock");
		}

		if (isHost()) {
			doEntityLockAction(targetId, getMyPeerId(), false, lockType, destroyOnUnlock);
			myLocks.erase(targetId);
		} else {
			auto entityNetworkSession = getSessionService().getMultiplayerSession().getEntityNetworkSession();
			if (!entityNetworkSession || !entityNetworkSession->isConnected()) {
				myLocks.erase(targetId);
				return;
			}

			sendMessage(NetworkEntityLockSystemMessage(targetId, false, static_cast<uint8_t>(lockType), destroyOnUnlock, getMyPeerId()), [=, this] (ConfigNode result) mutable
            {
				bool success = result["success"].asBool(false);

				if (lockType == NetworkLockType::UniqueWithAuthority) {
					if (!success) {
						Logger::logWarning("client failed to tell host to release lock, with authority, for entity ID " + toString(targetId.value & 0xffffffff));
					}
					auto [ok, _] = changeAuthority(targetId, {}, {});
					if (!ok) {
						Logger::logWarning("client failed to release lock, with authority, for entity ID " + toString(targetId.value & 0xffffffff));
					}
				}

				myLocks.erase(targetId);
            });
		}
	}

	std::pair<bool, std::optional<EntityNetworkId>> doEntityLockAction(EntityId targetId, NetworkSession::PeerId peerId, bool wantsToLock, NetworkLockType lockType, bool destroyOnUnlock)
	{
		if (!targetId.isValid()) {
			if (wantsToLock) {
				Logger::logDev("Peer tries to lock entity, but ID is invalid");
			} else if (destroyOnUnlock) {
				Logger::logDev("Peer tries to unlock entity, with destroyOnUnlock flag set, but ID is invalid");
			}
			return {false, {}};
		}

		const auto* e = tryFindNetworkRoot(targetId);
		if (!e) {
			// Entity not found
			if (const auto entity = getWorld().tryGetEntity(targetId); entity.isValid()) {
				Logger::logWarning("Peer tries to lock entity " + entity.getName() + " which isn't a network entity");
			}
			return {false, {}};
		}

		if (e->network.ownerId.value_or(0) != getMyPeerId()) {
			Logger::logError("Peer tries to lock or unlock entity " + getWorld().getEntity(targetId).getName() + " which isn't owned by host");
			return {false, {}};
		}

		if (wantsToLock) {
			return doEntityLock(e, peerId, lockType);
		} else {
			return doEntityUnlock(e, peerId, lockType, destroyOnUnlock);
		}
	}

	Vector<const NetworkFamily*> collectNetworkAncestorsAndDescendantsCached(ConstEntityRef e) const
	{
		Vector<const NetworkFamily*> result;
		result += &networkFamily.find(e.getEntityId());

		const auto& cache = getRemoteLockCache();
		for (const auto& r: cache) {
			if (r != e.getEntityId()) {
				auto other = getWorld().tryGetEntity(r);
				if (!other.isValid()) {
					continue;
				}

				if (e.hasEntityIdInAncestors(r) || other.hasEntityIdInAncestors(e.getEntityId())) {
					if (auto* n = networkFamily.tryFind(r)) {
						result += n;
					}
				}
			}
		}

		return result;
	}

	Vector<const NetworkFamily*> collectNetworkAncestorsAndDescendants(ConstEntityRef e) const
	{
		Vector<const NetworkFamily*> result;
		collectNetworkEntitiesInHierarchyUp(e, result); // Include self
		for (auto c: e.getChildren()) {
			collectNetworkEntitiesInHierarchyDown(c, result);
		}
		return result;
	}

	void collectNetworkEntitiesInHierarchyUp(ConstEntityRef e, Vector<const NetworkFamily*>& dst) const
	{
		if (e.hasComponent<NetworkComponent>()) {
			if (auto* n = networkFamily.tryFind(e.getEntityId())) {
				dst += n;
			}
		}
		if (e.hasParent()) {
			collectNetworkEntitiesInHierarchyUp(e.getParent(), dst);
		}
	}

	void collectNetworkEntitiesInHierarchyDown(ConstEntityRef e, Vector<const NetworkFamily*>& dst) const
	{
		if (e.hasComponent<NetworkComponent>()) {
			if (auto* n = networkFamily.tryFind(e.getEntityId())) {
				dst += n;
			}
		}
		for (auto c: e.getChildren()) {
			collectNetworkEntitiesInHierarchyDown(c, dst);
		}
	}

	std::pair<bool, std::optional<EntityNetworkId>> doEntityLock(const NetworkFamily* e, NetworkSession::PeerId peerId, NetworkLockType lockType)
	{
		std::pair<bool, std::optional<EntityNetworkId>> result = {true, {}};

		const auto ns = collectNetworkAncestorsAndDescendants(getWorld().getEntity(e->entityId));

		// Figure out the current lock situation
		bool someoneHasSharedLock = false;
		bool someoneHasExclusiveLock = false;
		NetworkLockInfo* myLock = nullptr;
		for (const auto* n: ns) {
			auto& locks = n->network.locks;
			for (auto& existingLock: locks) {
				if (existingLock.peerId == peerId) {
					if (n == e) { // Only do this for the target entity
						myLock = &existingLock;
					}
				} else {
					if (existingLock.type == NetworkLockType::Shared) {
						someoneHasSharedLock = true;
					} else {
						someoneHasExclusiveLock = true;
					}
				}
			}
		}

		// Someone has a lock that clashes with this
		const bool targetLockValid = !someoneHasExclusiveLock && (!someoneHasSharedLock || lockType == NetworkLockType::Shared);
		if (!targetLockValid) {
			return {false, {}};
		}

		// Try to update my existing lock
		if (myLock) {
			notifyEntityModified(e);
			if (lockType != myLock->type) {
				if (lockType < myLock->type) {
					// Downgrading lock type is not supported (could lead to inconsistencies?)
					return {false, {}};
				}

	            if (lockType == NetworkLockType::UniqueWithAuthority) {
	                // Try changing authority first. If this fails, don't bother creating a lock.
	                result = doChangeAuthority(e, peerId, {});
	                if (!result.first) {
	                    return result;
	                }
	            }
				myLock->type = lockType;
			}

			// Wants to lock again
			result.first = true;
			return result;
		}

		// New lock
		notifyEntityModified(e);
        if (lockType == NetworkLockType::UniqueWithAuthority) {
            // Try changing authority first. If this fails, don't bother creating a lock.
            result = doChangeAuthority(e, peerId, {});
            if (!result.first) {
                return result;
            }
        }
		e->network.locks.emplace_back(peerId, lockType);
		return result;
	}

	std::pair<bool, std::optional<EntityNetworkId>> doEntityUnlock(const NetworkFamily* e, NetworkSession::PeerId peerId, NetworkLockType lockType, bool destroyOnUnlock)
	{
		std::pair<bool, std::optional<EntityNetworkId>> result = {true, {}};
		auto& locks = e->network.locks;

		if (locks.empty()) {
			// Tries to unlock non-existing lock
			return {false, {}};
		}

		for (size_t i = 0; i < locks.size(); ++i) {
			auto& existingLock = locks[i];
			if (existingLock.peerId == peerId) {
				if (existingLock.type != lockType) {
					Logger::logError("Attempting to unlock lock with different type from acquired");
				}

				// Lock exists, locked by this peer
				notifyEntityModified(e);

				// Release lock
                if (existingLock.type == NetworkLockType::UniqueWithAuthority) {
                    // Release authority first. If this fails, keep the lock.
                    result = doChangeAuthority(e, {}, {});
                    if (!result.first) {
                        return result;
                    }
                }
				locks.erase(locks.begin() + i);
				if (locks.empty() && destroyOnUnlock) {
					getWorld().destroyEntity(e->entityId);
				}

				return result;
			}
		} 

		// Lock exists, locked by someone else
		return {false, {}};
	}

	[[nodiscard]] NetworkSession::PeerId getMyPeerId() const
	{
		if (!getSessionService().isMultiplayer()) {
			return 0;
		}
		auto& mpSession = getSessionService().getMultiplayerSession();
		return mpSession.getNetworkSession()->getMyPeerId().value_or(0);
	}

	[[nodiscard]] bool isHost() const
	{
		if (!getSessionService().isMultiplayer()) {
			return true;
		}
		return getSessionService().getMultiplayerSession().isHost();
	}

	[[nodiscard]] bool isPeerPresent(NetworkSession::PeerId peerId) const
	{
		if (peerId == getMyPeerId()) {
			return true;
		}
		return getSessionService().isMultiplayer() && std_ex::contains(getSessionService().getMultiplayerSession().getNetworkSession()->getRemotePeers(), peerId);
	}

    [[nodiscard]] std::pair<bool, std::optional<EntityNetworkId>> changeAuthority(EntityId targetId,
    	const std::optional<NetworkSession::PeerId>& authorityId, const std::optional<EntityNetworkId>& assignNetworkId)
    {
        if (!targetId.isValid()) {
            Logger::logWarning("Trying to change authority of invalid entity.");
            return {false, {}};
        }

        const auto* e = tryFindNetworkRoot(targetId);
        if (!e) {
            Logger::logWarning("Trying to change authority of entity " + toString(targetId.value & 0xffffffff) + " which is unknown, or not a network entity");
            return {false, {}};
        }

		return doChangeAuthority(e, authorityId, assignNetworkId);
    }

    [[nodiscard]] std::pair<bool, std::optional<EntityNetworkId>> doChangeAuthority(const NetworkFamily* networkFamily,
    	const std::optional<NetworkSession::PeerId>& authorityId, const std::optional<EntityNetworkId>& assignNetworkId) const
    {
		std::pair<bool, std::optional<EntityNetworkId>> result = {true, {}};

		if (getSessionService().isMultiplayer()) {
			auto entityNetworkSession = getSessionService().getMultiplayerSession().getEntityNetworkSession();

			result = entityNetworkSession->prepareChangeEntityAuthority(
				networkFamily->entityId, networkFamily->network, authorityId, assignNetworkId);

			if (result.first) {
				notifyEntityModified(networkFamily);
			} else {
				if (authorityId.has_value()) {
		            Logger::logWarning("Failed to assign authority of entity " +
		            	toString(networkFamily->entityId.value & 0xffffffff) + " to " +
		            	toString(static_cast<int>(authorityId.value())));
				} else {
					Logger::logWarning("Failed to release authority of entity " +
						toString(networkFamily->entityId.value & 0xffffffff) + " to " +
						toString(static_cast<int>(networkFamily->network.ownerId.value_or(0))));
				}

				return result;
			}
		}

		networkFamily->network.authorityId = authorityId;

		return result;
    }

	[[nodiscard]] EntityId resolveLockTarget(EntityId id, bool useFallback) const
	{
	    const auto* root = tryFindNetworkRoot(id);
		if (!root && !useFallback) {
			Logger::logError("NetworkLockSystem failed to find root for entity " + id.toDetailedString());
			return {};
		}
	    return root ? root->entityId : id;
	}

	[[nodiscard]] const NetworkFamily* tryFindNetworkRoot(EntityId entityId) const
	{
		if (const auto* e = networkFamily.tryFind(entityId)) {
			return e;
		}

		const auto entityRef = tryFindNetworkRoot(getWorld().tryGetEntity(entityId));
		return entityRef.isValid() ? networkFamily.tryFind(entityRef.getEntityId()) : nullptr;
	}

	[[nodiscard]] static ConstEntityRef tryFindNetworkRoot(ConstEntityRef entityRef)
	{
		if (!entityRef.isValid()) {
			return {};
		}

		if (entityRef.hasComponent<NetworkComponent>()) {
			return entityRef;
		}

		if (entityRef.hasParent()) {
			return tryFindNetworkRoot(entityRef.getParent());
		}

		return {};
	}

	void notifyEntityModified(const NetworkFamily* e) const {
		// NB: On the host, this "wakes up" a dormant network entity if its locks are modified.
		// Scripts tend to lock entities as a first step, or very early in the process, so this
		// kind of auto-enables support of this feature, without requiring much more attention
		// in source code (like, in more complex entity systems, to attend all cases where we
		// need to call this to signal a modifications).
		auto entity = getWorld().getEntity(e->entityId);
		entity.setModifiedThisFrame(true, true);

		hasRemoteLockCache = false;
	}

};

REGISTER_SYSTEM(NetworkLockSystem)
