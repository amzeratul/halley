#pragma once

#include "halley/api/audio_api.h"
#include "halley/text/halleystring.h"
#include "halley/concurrency/future.h"
#include "halley/entity/entity_id.h"

namespace Halley {
	class Animation;
	class WorldPosition;
	struct alignas(8) EntityId;
	class LuaState;
	class LuaReference;
	class LuaExpression;
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

    	virtual ~INetworkLockSystemInterface() override = default;

        virtual LockStatus getLockStatus(EntityId targetId) const = 0;
        virtual bool isLockedByOrAvailableTo(EntityId playerId, EntityId targetId) const = 0;
        virtual Future<NetworkLockHandle> lockAcquire(EntityId playerId, EntityId targetId, bool acquireAuthority) = 0;
	};

    class ILuaInterface : public ISystemInterface {
    public:
        virtual LuaState& getLuaState() const = 0;
		virtual LuaReference& getLuaReference(const LuaExpression& luaExpression) const = 0;
    };

	class IScriptSystemInterface : public ISystemInterface {
	public:
		virtual ~IScriptSystemInterface() override = default;

		virtual std::shared_ptr<ScriptState> addScript(EntityId target, const String& scriptId, Vector<String> tags, Vector<ConfigNode> params) = 0;
		virtual bool stopScript(EntityId target, const String& scriptId, bool allThreads = false) = 0;
		virtual bool stopTag(EntityId target, const String& tagId, const String& exceptScriptId, bool allThreads = false) = 0;
		virtual bool stopTag(EntityId target, const String& tagId, const Vector<String>& exceptScriptIds = {}, bool allThreads = false) = 0;
		virtual void setLoadedScriptsWithTag(EntityId target, const String& tagId, const Vector<String>& scriptIds) = 0;
		virtual bool isRunningScript(EntityId target, const String& scriptId) = 0;
		virtual void resetStartedScripts(EntityId target) = 0;

		virtual void sendReturnHostThread(EntityId target, const String& scriptId, int node, ConfigNode params) = 0;
		virtual void startHostThread(EntityId entityId, const String& scriptId, int nodeId, ConfigNode params) = 0;
		virtual void cancelHostThread(EntityId entityId, const String& scriptId, int nodeId) = 0;
	};

	class IScriptableQuerySystemInterface : public ISystemInterface {
	public:
		using Callback = std::function<Vector<std::pair<EntityId, gsl::span<const String>>>()>;

		virtual ~IScriptableQuerySystemInterface() override = default;

		virtual Vector<EntityId> findEntities(WorldPosition pos, float radius, int limit, const Vector<String>& tags, const Vector<String>& components, const std::function<float(EntityId, WorldPosition)>& getDistance) const = 0;
		virtual void setFindEntitiesCallback(Callback callback) = 0;
	};

	class IAudioSystemInterface : public ISystemInterface {
	public:
		virtual ~IAudioSystemInterface() override = default;

        virtual void playAudio(const String& event, EntityId entityId) = 0;
        virtual void playAudio(const String& event, WorldPosition position, std::optional<AudioRegionId> regionId) = 0;
		virtual void setGlobalVariable(const String& variableName, float value) = 0;
		virtual void setVariable(EntityId entityId, const String& variableName, float value) = 0;
		virtual std::optional<String> getSourceName(AudioEmitterId id) const = 0;
		virtual String getRegionName(AudioRegionId id) const = 0;
		virtual void setRegionLookup(std::function<AudioRegionId(WorldPosition pos)> f) = 0;
		virtual void addEmitterNameLookup(std::function<std::optional<String>(AudioEmitterId)> f) = 0;
	};

	class IExitGameInterface : public ISystemInterface {
	public:
		virtual ~IExitGameInterface() override = default;

		virtual void exitGame() = 0;
	};

	class IEnableRulesSystemInterface : public ISystemInterface {
	public:
		virtual ~IEnableRulesSystemInterface() override = default;

		virtual void refreshEnabled() = 0;
	};

	class SpriteAnimationEvent {
	public:
		EntityId entityId;
		gsl::span<const String> tags;
		const Animation* animation = nullptr;
		std::string_view sequenceName;
		int direction;
		int frame;
	};

	class ISpriteAnimationSystemInterface : public ISystemInterface {
	public:
		using Callback = std::function<void(const SpriteAnimationEvent&)>;

		virtual ~ISpriteAnimationSystemInterface() override = default;

		virtual void setCallback(Callback callback) = 0;
	};
}
