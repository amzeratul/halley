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
		sessionChangedToken = getSessionService().addSessionChangeCallback([=] (SessionService::ChangeData data) {
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
	}

	[[nodiscard]] LockStatus getLockStatus(EntityId targetId) const override
	{
		if (const NetworkFamily* e = tryFindNetworkRoot(targetId)) {
			const auto iter = std_ex::find_if(e->network.locks, [&](const auto& lock) { return lock.first == targetId; });
			if (iter != e->network.locks.end()) {
				return iter->second == getMyPeerId() ? LockStatus::AcquiredByMe : LockStatus::AcquiredByOther;
			}
		} else {
			//Logger::logError("Trying to get lock status of unknown network entity " + toString(targetId));
			return LockStatus::Unlocked;
		}
		return LockStatus::Unlocked;
	}

	[[nodiscard]] bool isLockedByOrAvailableTo(EntityId playerId, EntityId targetId) const override
	{
		if (const NetworkFamily* e = tryFindNetworkRoot(targetId)) {
			const auto iter = std_ex::find_if(e->network.locks, [&](const auto& lock) { return lock.first == targetId; });
			if (iter != e->network.locks.end()) {
				if (const NetworkFamily* playerEntity = networkFamily.tryFind(playerId)) {
					const auto playerPeer = playerEntity->network.ownerId.value_or(0);
					return iter->second == playerPeer;
				}
				Logger::logWarning("Couldn't find locker entity");
				return false;
			}
		} else if (getSessionService().isMultiplayer()) {
			const auto entity = getWorld().tryGetEntity(targetId);
			if (entity.isValid()) {
				Logger::logWarning("Trying to get lock status of non-network entity \"" + entity.getName() + "\" (" + toString(entity.getEntityId().value & 0xffffffff) + ") (missing NetworkComponent?)", true);
			} else if (targetId.isValid()) {
				Logger::logWarning("Trying to get lock status of unknown entity " + toString(targetId.value & 0xffffffff), true);
			}
			return true;
		}
		return true;
	}

	Future<NetworkLockHandle> lockAcquire(EntityId playerId, EntityId targetId, bool acquireAuthority) override
	{
		const auto iter = myLocks.find(targetId);
		if (iter != myLocks.end()) {
			// Locked by some local entity
			if (iter->second.playerId == playerId) {
				// Already locked by this player, just increment count!
				iter->second.refCount++;
                if (acquireAuthority && !iter->second.withAuthority && getSessionService().isMultiplayer()) {
                    Logger::logWarning("Tried to acquire lock, with authority, for entity already locked");
                }
				return Future<NetworkLockHandle>::makeImmediate(std::make_shared<NetworkLock>(static_cast<INetworkLockSystem&>(*this), playerId, targetId));
			} else {
				// Locked by someone else
				return Future<NetworkLockHandle>::makeImmediate({});
			}
		}

		// Not locked locally, try to lock
		return doLockAcquireForMe(targetId, acquireAuthority).then([=] (bool success) -> NetworkLockHandle
		{
			if (success) {
				auto& l = myLocks[targetId];
				l.playerId = playerId;
				l.refCount++;
                l.withAuthority = acquireAuthority;
				return std::make_shared<NetworkLock>(static_cast<INetworkLockSystem&>(*this), playerId, targetId);
			}
			return {};
		});
	}

	void lockUpdate(EntityId targetId, bool destroyOnUnlock) override
	{
		const auto iter = myLocks.find(targetId);
		if (iter != myLocks.end()) {
			iter->second.destroyOnUnlock = destroyOnUnlock;
		}
	}

	void lockRelease(EntityId playerId, EntityId targetId) override
	{
		const auto iter = myLocks.find(targetId);
		if (iter != myLocks.end()) {
			auto& l = iter->second;
			if (l.playerId == playerId) {
				l.refCount--;
				if (l.refCount == 0) {
					doLockReleaseForMe(targetId, l.withAuthority, l.destroyOnUnlock);
					myLocks.erase(iter);
				}
			} else {
				Logger::logError("Releasing network lock with handle that isn't locking it!");
			}
		} else {
			Logger::logError("Releasing network lock for entity that isn't locked!");
		}		
	}

	ConfigNode onMessageReceived(NetworkEntityLockSystemMessage msg) override
	{
		const auto [success, networkId] = doEntityLock(msg.target, msg.peerId, msg.lock, msg.withAuthority, msg.destroyOnUnlock);

		ConfigNode::MapType result;
		result["success"] = success;
		if (networkId.has_value()) {
			result["id"] = networkId.value();
		}

		return result;
	}

private:
	struct LocalLock {
		EntityId playerId;
		int refCount = 0;
        bool withAuthority = false;
		bool destroyOnUnlock = false;
	};
	HashMap<EntityId, LocalLock> myLocks;
	ListenerSetToken sessionChangedToken;

	void onPeerDisconnected(NetworkSession::PeerId peerId) override
	{
		checkStaleLocks(peerId);
		sendMessage(NetworkPeerDisconnectSystemMessage(peerId));
	}

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
				std_ex::erase_if_value(e.network.locks, predicate);
			}
		}
	}

	Future<bool> doLockAcquireForMe(EntityId targetId, bool withAuthority)
	{
		if (isHost()) {
			auto [success, networkId] = doEntityLock(targetId, getMyPeerId(), true, withAuthority, false);
			HalleyAssertDebug(!networkId.has_value());
			return Future<bool>::makeImmediate(success);
		} else {
			Promise<bool> promise;
			auto future = promise.getFuture();
			sendMessage(NetworkEntityLockSystemMessage(targetId, true, withAuthority, false, getMyPeerId()), [=, promise = std::move(promise)] (ConfigNode result) mutable
			{
				bool success = result["success"].asBool(false);

				std::optional<EntityNetworkId> assignNetworkId;
				if (result.hasKey("id")) {
					assignNetworkId = result["id"].asInt();
				}

                if (success && withAuthority) {
                    auto [ok, networkId] = changeAuthority(targetId, getMyPeerId(), assignNetworkId);
                	HalleyAssertDebug(!networkId.has_value());
                	success = ok;
                }

				promise.setValue(success);
			});
			return future;
		}
	}

	void doLockReleaseForMe(EntityId targetId, bool withAuthority, bool destroyOnUnlock)
	{
		if (isHost()) {
			doEntityLock(targetId, getMyPeerId(), false, withAuthority, destroyOnUnlock);
		} else {
			sendMessage(NetworkEntityLockSystemMessage(targetId, false, withAuthority, destroyOnUnlock, getMyPeerId()), [=] (ConfigNode result) mutable
            {
				bool success = result["success"].asBool(false);

				if (withAuthority) {
					if (!success) {
						Logger::logWarning("client failed to tell host to release lock, with authority, for entity ID " + toString(targetId.value & 0xffffffff));
					}
					auto [ok, _] = changeAuthority(targetId, {}, {});
					if (!ok) {
						Logger::logWarning("client failed to release lock, with authority, for entity ID " + toString(targetId.value & 0xffffffff));
					}
				}
            });
		}
	}

	std::pair<bool, std::optional<EntityNetworkId>> doEntityLock(EntityId targetId, NetworkSession::PeerId peerId, bool lock, bool withAuthority, bool destroyOnUnlock)
	{
		std::pair<bool, std::optional<EntityNetworkId>> result = {true, {}};

		if (!targetId.isValid()) {
			Logger::logDev("Peer attempted to lock invalid entity.");
			return {false, {}};
		}

		const auto* e = tryFindNetworkRoot(targetId);
		if (e) {
			if (e->network.ownerId.value_or(0) != getMyPeerId()) {
				Logger::logError("Peer attempted to lock or unlock entity " + getWorld().getEntity(targetId).getName() + " which isn't owned by host.");
				return {false, {}};
			}

			auto& locks = e->network.locks;
			const auto iter = std_ex::find_if(e->network.locks, [&](const auto& e) { return e.first == targetId; });

			if (iter == locks.end()) {
				// No existing lock
				if (lock) {
					// New lock
                    if (withAuthority) {
                    	// Try changing authority first. If this fails, don't bother creating a lock.
                    	result = doChangeAuthority(e, peerId, {});
                        if (!result.first) {
	                        return result;
                        }
                    }
					locks.emplace_back(targetId, peerId);
					return result;
				} else {
					// Tries to unlock non-existing lock
					return {false, {}};
				}
			} else if (iter->second == peerId) {
				// Lock exists, locked by this peer
				if (!lock) {
					// Release lock
                    if (withAuthority) {
                    	// Release authority first. If this fails, keep the lock.
                    	result = doChangeAuthority(e, {}, {});
                        if (!result.first) {
	                        return result;
                        }
                    }
					locks.erase(iter);
					if (destroyOnUnlock) {
						getWorld().destroyEntity(targetId);
					}
					return result;
				} else {
					// Wants to lock again
					result.first = true;
					return result;
				}
			} else {
				// Lock exists, locked by someone else
				return {false, {}};
			}
		} else {
			// Entity not found
			if (const auto entity = getWorld().tryGetEntity(targetId); entity.isValid()) {
				Logger::logWarning("Peer attempted to lock entity " + entity.getName() + " which isn't a network entity");
			}
			return {false, {}};
		}
	}

	NetworkSession::PeerId getMyPeerId() const
	{
		if (!getSessionService().isMultiplayer()) {
			return 0;
		}
		auto& mpSession = getSessionService().getMultiplayerSession();
		return mpSession.getNetworkSession()->getMyPeerId().value_or(0);
	}

	bool isHost() const
	{
		if (!getSessionService().isMultiplayer()) {
			return true;
		}
		return getSessionService().getMultiplayerSession().isHost();
	}

	bool isPeerPresent(NetworkSession::PeerId peerId)
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

			if (!result.first) {
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
};

REGISTER_SYSTEM(NetworkLockSystem)
