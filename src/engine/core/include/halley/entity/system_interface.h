#pragma once

#include "halley/text/halleystring.h"
#include "halley/concurrency/future.h"

namespace Halley {
	class WorldPosition;
	struct alignas(8) EntityId;
	class LuaState;
	class ScriptState;
	class ConfigNode;

	class ISystemInterface {
	public:
		virtual ~ISystemInterface() = default;
	};

    class INetworkLock {
    public:
        virtual ~INetworkLock() = default;
    };

    using NetworkLockHandle = std::shared_ptr<INetworkLock>;

    class INetworkLockSystemInterface : public ISystemInterface {
	public:
        enum class LockStatus {
	        Unlocked,
            AcquiredByMe,
            AcquiredByOther
        };

    	virtual ~INetworkLockSystemInterface() = default;

        virtual LockStatus getLockStatus(EntityId playerId, EntityId targetId) const = 0;
        virtual Future<NetworkLockHandle> lockAcquire(EntityId playerId, EntityId targetId) = 0;
	};

    class ILuaInterface : public ISystemInterface {
    public:
        virtual LuaState& getLuaState() = 0;
    };

	class IScriptSystemInterface : public ISystemInterface {
	public:
		virtual ~IScriptSystemInterface() = default;

		virtual Vector<EntityId> findScriptables(WorldPosition pos, float radius, int limit, const Vector<String>& tags, const std::function<float(EntityId, WorldPosition)>& getDistance) const = 0;
		virtual std::shared_ptr<ScriptState> addScript(EntityId target, const String& scriptId, Vector<String> tags, Vector<ConfigNode> params) = 0;
		virtual void sendReturnHostThread(EntityId target, const String& scriptId, int node) = 0;
		virtual void startHostThread(EntityId entityId, const String& scriptId, int nodeId) = 0;
		virtual void cancelHostThread(EntityId entityId, const String& scriptId, int nodeId) = 0;
	};

	class IAudioSystemInterface : public ISystemInterface {
	public:
		virtual ~IAudioSystemInterface() = default;

        virtual void playAudio(const String& event, EntityId entityId) = 0;
	};
}
