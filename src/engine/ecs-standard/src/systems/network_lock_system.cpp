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

		setupSession();
		sessionChangedToken = getSessionService().addSessionChangeCallback([=] (std::shared_ptr<Session> session) {
			// This doesn't unregister from the last one, which should be fine since it's destroyed by this point anyway
			setupSession();
		});
	}

	void deInit() override
	{
		if (const auto session = getEntityNetworkSession()) {
			session->getSession().removeListener(this);
		}
	}

	void setupSession()
	{
		if (const auto session = getEntityNetworkSession()) {
			session->getSession().addListener(this);
		}
	}

	void update(Time t)
	{
		checkStaleLocks({});
	}

	LockStatus getLockStatus(EntityId targetId) const override
	{
		if (const NetworkFamily* e = getRootEntity(targetId)) {
			const auto iter = std_ex::find_if(e->network.locks, [&](const auto& e) { return e.first == targetId; });
			if (iter != e->network.locks.end()) {
				return iter->second == getMyPeerId() ? LockStatus::AcquiredByMe : LockStatus::AcquiredByOther;
			}
		} else {
			return LockStatus::Unlocked;
			//Logger::logError("Trying to get lock status of unknown network entity " + toString(targetId));
		}
		return LockStatus::Unlocked;
	}

	bool isLockedByOrAvailableTo(EntityId playerId, EntityId targetId) const override
	{
		if (const NetworkFamily* e = getRootEntity(targetId)) {
			const auto iter = std_ex::find_if(e->network.locks, [&](const auto& e) { return e.first == targetId; });
			if (iter != e->network.locks.end()) {
				if (const NetworkFamily* playerEntity = getRootEntity(playerId)) {
					const auto playerPeer = playerEntity->network.ownerId.value_or(0);
					return iter->second == playerPeer;
				} else {
					Logger::logWarning("Couldn't find locker entity");
					return false;
				}
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
                if (iter->second.withAuthority != acquireAuthority) {
                    Logger::logWarning("Trying to acquire lock to already locked entity, but with different authority flag");
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

	void lockRelease(EntityId playerId, EntityId targetId) override
	{
		const auto iter = myLocks.find(targetId);
		if (iter != myLocks.end()) {
			auto& l = iter->second;
			if (l.playerId == playerId) {
				l.refCount--;
				if (l.refCount == 0) {
					doLockReleaseForMe(targetId, l.withAuthority);
					myLocks.erase(iter);
				}
			} else {
				Logger::logError("Releasing network lock with handle that isn't locking it!");
			}
		} else {
			Logger::logError("Releasing network lock for entity that isn't locked!");
		}		
	}

	bool onMessageReceived(NetworkEntityLockSystemMessage msg) override
	{
		return doEntityLock(msg.target, msg.peerId, msg.lock, msg.withAuthority);
	}

private:
	struct LocalLock {
		EntityId playerId;
		int refCount = 0;
        bool withAuthority = false;
	};
	HashMap<EntityId, LocalLock> myLocks;
	ListenerSetToken sessionChangedToken;

	const EntityNetworkSession* getEntityNetworkSession() const
	{
		if (getSessionService().isMultiplayer()) {
			return getSessionService().getMultiplayerSession().getEntityNetworkSession();
		}
		return nullptr;
	}

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
					doChangeAuthority(&e, {});
				}
				// Erase any stale locks.
				std_ex::erase_if_value(e.network.locks, predicate);
			}
		}
	}

	Future<bool> doLockAcquireForMe(EntityId targetId, bool withAuthority)
	{
		if (isHost()) {
			return Future<bool>::makeImmediate(doEntityLock(targetId, getMyPeerId(), true, withAuthority));
		} else {
			/*auto target = getWorld().tryGetEntity(targetId);
			if (target.isValid()) {
				Logger::logDev("- acquire lock for me, id=" + toString(targetId.value & 0xffffffff) + ", " + target.getName());
			} else {
				Logger::logDev("- acquire lock for me, but no entity found, id=" + toString(targetId.value & 0xffffffff));
			}*/
			Promise<bool> promise;
			auto future = promise.getFuture();
			sendMessage(NetworkEntityLockSystemMessage(targetId, true, withAuthority, getMyPeerId()), [=, promise = std::move(promise)] (bool value) mutable
			{
                if (value && withAuthority) {
                    changeAuthority(targetId, getMyPeerId());
                }
				promise.setValue(value);
			});
			return future;
		}
	}

	void doLockReleaseForMe(EntityId targetId, bool withAuthority)
	{
		if (isHost()) {
			doEntityLock(targetId, getMyPeerId(), false, withAuthority);
		} else {
			/*auto target = getWorld().tryGetEntity(targetId);
			if (target.isValid()) {
				Logger::logDev("- release lock for me, id=" + toString(targetId.value & 0xffffffff) + ", " + target.getName());
			} else {
				Logger::logDev("- release lock for me, but target entity not found, id=" + toString(targetId.value & 0xffffffff));
			}*/
			if (withAuthority) {
				// On a peer, give up authority right away, no matter if the network message below
				// is delivered successfully or not.
				// This avoids some confusion with entity network updates working on "wrong" state
				// while the message to change authority itself is still in flight.
				changeAuthority(targetId, {});
			}
			sendMessage(NetworkEntityLockSystemMessage(targetId, false, withAuthority, getMyPeerId()), [=] (bool value) mutable
            {
				if (!value && withAuthority) {
	                Logger::logWarning("client failed to tell host to release lock, with authority, for entity ID " + toString(targetId));
				}
            });
		}
	}

	bool doEntityLock(EntityId targetId, NetworkSession::PeerId peerId, bool lock, bool withAuthority)
	{
		if (!targetId.isValid()) {
			Logger::logDev("Peer attempted to lock invalid entity.");
			return false;
		}

		const auto* e = getRootEntity(targetId);
		if (e) {
			if (e->network.ownerId.value_or(0) != getMyPeerId()) {
				Logger::logError("Peer attempted to lock entity " + getWorld().getEntity(targetId).getName() + " which isn't owned by host.");
				return false;
			}

			auto& locks = e->network.locks;
			const auto iter = std_ex::find_if(e->network.locks, [&](const auto& e) { return e.first == targetId; });

			if (iter == locks.end()) {
				// No existing lock
				if (lock) {
					// New lock
					locks.emplace_back(targetId, peerId);
                    if (withAuthority) {
                        doChangeAuthority(e, peerId);
                    }
					return true;
				} else {
					// Tries to unlock non-existing lock
					return false;
				}
			} else if (iter->second == peerId) {
				// Lock exists, locked by this peer
				if (!lock) {
					// Release lock
					locks.erase(iter);
                    if (withAuthority) {
                        doChangeAuthority(e, {});
                    }
					return true;
				} else {
					// Wants to lock again
					return true;
				}
			} else {
				// Lock exists, locked by someone else
				return false;
			}
		} else {
			// Entity not found
			const auto entity = getWorld().tryGetEntity(targetId);
			if (entity.isValid()) {
				Logger::logWarning("Peer attempted to lock entity " + entity.getName() + " which isn't a network entity or descendant of one.");
			}
			return false;
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

	const NetworkFamily* getRootEntity(EntityId targetId) const
	{
		const auto root = findNetworkRoot(getWorld().tryGetEntity(targetId));
		if (!root.isValid()) {
			return nullptr;
		}
		return networkFamily.tryFind(root.getEntityId());
	}

	EntityRef findNetworkRoot(EntityRef e) const
	{
		if (!e.isValid()) {
			return {};
		}
		if (e.hasComponent<NetworkComponent>()) {
			return e;
		}
		if (e.hasParent()) {
			return findNetworkRoot(e.getParent());
		}
		return {};
	}

    void changeAuthority(EntityId targetId, std::optional<NetworkSession::PeerId> authorityId)
    {
        if (!targetId.isValid()) {
            Logger::logWarning("Trying to change authority of invalid entity.");
            return;
        }

        const auto* e = getRootEntity(targetId);
        if (e) {
            doChangeAuthority(e, authorityId);
        } else {
            Logger::logWarning("Trying to change authority of entity " + toString(targetId) + " which is unknown, or not a network entity");
        }
    }

    void doChangeAuthority(const NetworkFamily* networkFamily, std::optional<NetworkSession::PeerId> authorityId)
    {
		if (getSessionService().isMultiplayer()) {
			auto entityNetworkSession = getSessionService().getMultiplayerSession().getEntityNetworkSession();
			if (!entityNetworkSession->prepareChangeEntityAuthority(networkFamily->entityId, networkFamily->network, authorityId)) {
				Logger::logWarning("Network session failed to change authority for local entity");
			}
		}

		networkFamily->network.authorityId = authorityId;
    }
};

REGISTER_SYSTEM(NetworkLockSystem)
