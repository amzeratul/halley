#pragma once
#include <typeinfo>
#include <typeindex>

#include "script_message.h"
#include "halley/entity/entity_factory.h"
#include "script_node_type.h"
#include "halley/input/input_virtual.h"

namespace Halley {
	class LuaState;
	class UIWidget;
	class InputDevice;
	class ScriptState;

    enum class ScriptVariableScope {
        Local,
        Shared,
        Entity
    };

	template <>
	struct EnumNames<ScriptVariableScope> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"local",
                "shared",
				"entity"
			}};
		}
	};
    	    
	class IScriptEnvironmentInterface {
	public:
		virtual ~IScriptEnvironmentInterface() = default;
	};

    class INetworkLock {
    public:
        virtual ~INetworkLock() = default;
    };

    using NetworkLockHandle = std::shared_ptr<INetworkLock>;

    class INetworkLockSystemInterface : public IScriptEnvironmentInterface {
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

    class ILuaInterface : public IScriptEnvironmentInterface {
    public:
        virtual LuaState& getLuaState() = 0;
    };

    class ScriptEnvironment: private IEntityFactoryContext {
    public:
        struct EntityMessageData {
	        EntityId targetEntity;
            String messageName;
            ConfigNode messageData;
        };

        struct SystemMessageData {
	        String targetSystem;
            String messageName;
            ConfigNode messageData;
        };

        enum class ScriptExecutionRequestType {
	        Start,
            Stop,
            StopTag
        };

        struct ScriptExecutionRequest {
	        ScriptExecutionRequestType type;
            EntityId target;
            String value;
            Vector<String> startTags;
            Vector<ConfigNode> params;
        };

        using ScriptTargetRetriever = std::function<EntityId(const String&)>;

    	ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources, const ScriptNodeTypeCollection& nodeTypeCollection);
    	virtual ~ScriptEnvironment() = default;

    	virtual void update(Time time, ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables);
    	void terminateState(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables);
		ConfigNode readNodeElementDevConData(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables, GraphNodeId nodeId, GraphPinId pinId);

    	EntityRef tryGetEntity(EntityId entityId) const;
    	const ScriptGraph* getCurrentGraph() const;
        size_t& getNodeCounter(GraphNodeId nodeId);
        IScriptStateData* getNodeData(GraphNodeId nodeId);
        void assignTypes(const ScriptGraph& graph);

    	ConfigNode readInputDataPin(const ScriptGraphNode& node, GraphPinId pinN);
        ConfigNode readOutputDataPin(const ScriptGraphNode& node, GraphPinId pinN);
        EntityId readInputEntityIdRaw(const ScriptGraphNode& node, GraphPinId pinN);
        EntityId readInputEntityId(const ScriptGraphNode& node, GraphPinId pinN);
        EntityId readOutputEntityId(const ScriptGraphNode& node, GraphPinId pinN);

    	void postAudioEvent(const String& id, EntityId entityId);

        ScriptVariables& getVariables(ScriptVariableScope scope);
        const ScriptVariables& getVariables(ScriptVariableScope scope) const;
        const ScriptVariables& getEntityVariables(EntityId entityId) const;

    	virtual void setDirection(EntityId entityId, const String& direction);

        void setInputEnabled(bool enabled);
        virtual bool isInputEnabled() const;
        void setInputDevice(EntityId target, std::shared_ptr<InputVirtual> input);
        std::shared_ptr<InputVirtual> getInputDevice(EntityId target, bool bypassEnableCheck) const;
        virtual int getInputButtonByName(const String& name) const;
        bool hasInputLabel(EntityId entityId) const;

        template <typename T>
        T* tryGetComponent(EntityId id)
        {
	        auto entity = tryGetEntity(id);
            return entity.isValid() ? entity.tryGetComponent<T>() : nullptr;
        }

        void setHostNetworkAuthority(bool isHost);
        bool hasNetworkAuthorityOver(EntityId id);
        bool hasHostNetworkAuthority() const;

    	int getCurrentFrameNumber() const;
        Time getDeltaTime() const;
        EntityId getCurrentEntityId() const override;
        GraphPinId getCurrentInputPin() const;

        World& getWorld();
        Resources& getResources();

    	void sendScriptMessage(EntityId dstEntity, ScriptMessage message);
        void sendEntityMessage(EntityMessageData message);
        void sendSystemMessage(SystemMessageData message);

        void startScript(EntityId target, const String& scriptName, Vector<String> tags, Vector<ConfigNode> params);
        void stopScript(EntityId target, const String& scriptName);
        void stopScriptTag(EntityId target, const String& tag);

    	Vector<std::pair<EntityId, ScriptMessage>> getOutboundScriptMessages();
        Vector<EntityMessageData> getOutboundEntityMessages();
        Vector<ScriptExecutionRequest> getScriptExecutionRequests();

        virtual std::shared_ptr<UIWidget> createInWorldUI(const String& ui, Vector2f offset, Vector2f alignment, EntityId entityId);
        virtual std::shared_ptr<UIWidget> createModalUI(const String& ui, ConfigNode data, bool setAnchor = true);

        EntityId getScriptTarget(const String& id) const;
        void setScriptTargetRetriever(ScriptTargetRetriever scriptTargetRetriever);

        gsl::span<const ConfigNode> getStartParams() const;
        
		template <typename T>
		T& getInterface()
		{
			const auto iter = interfaces.find(std::type_index(typeid(T)));
			if (iter == interfaces.end()) {
				throw Exception(String("Script Environment does not have interface \"") + typeid(T).name() + "\"", HalleyExceptions::Scripting);
			}
			return dynamic_cast<T&>(*iter->second);
		}

		template <typename T>
		void setInterface(T* interface)
		{
			static_assert(std::is_abstract_v<T>);
			static_assert(std::is_base_of_v<IScriptEnvironmentInterface, T>);
			interfaces[std::type_index(typeid(T))] = interface;
		}

    protected:
		const HalleyAPI& api;
    	World& world;
    	Resources& resources;
        HashMap<EntityId, std::shared_ptr<InputVirtual>> inputDevices;
    	const ScriptNodeTypeCollection& nodeTypeCollection;
        bool isHost = false;
        bool inputEnabled = true;

        GraphPinId currentInputPin = 0;
    	const ScriptGraph* currentGraph = nullptr;
    	ScriptState* currentState = nullptr;
        ScriptVariables* currentEntityVariables = nullptr;
        EntityId currentEntity;
        Time deltaTime = 0;
        EntitySerializationContext serializationContext;

        Vector<std::pair<EntityId, ScriptMessage>> scriptOutbox;
        Vector<EntityMessageData> entityOutbox;
        Vector<ScriptExecutionRequest> scriptExecutionRequestOutbox;

    	HashMap<std::type_index, IScriptEnvironmentInterface*> interfaces;

        ScriptTargetRetriever scriptTargetRetriever;

    private:
        bool updateThread(ScriptState& graphState, ScriptStateThread& thread, Vector<ScriptStateThread>& pendingThreads);
        void terminateStateWith(const ScriptGraph* scriptGraph);
        void doTerminateState();
        void runDestructor(GraphNodeId nodeId);

        ScriptStateThread startThread(ScriptStateThread thread);
        void addThread(ScriptStateThread thread, Vector<ScriptStateThread>& pending);
        void advanceThread(ScriptStateThread& thread, OptionalLite<GraphNodeId> node, GraphPinId outputPin, GraphPinId inputPin);
        void initNode(GraphNodeId nodeId, ScriptState::NodeState& state);
        size_t forkThread(ScriptStateThread& thread, std::array<IScriptNodeType::OutputNode, 8> outputNodes, Vector<ScriptStateThread>& pendingThreads, size_t firstIdx = 0);
        void mergeThread(ScriptStateThread& thread, bool wait);
        void terminateThread(ScriptStateThread& thread, bool allowRollback);
        void removeStoppedThreads();
        void setWatcher(ScriptStateThread& thread, bool newState);

        void cancelOutputs(GraphNodeId nodeId, uint8_t cancelMask);
        void abortCodePath(GraphNodeId node, std::optional<GraphPinId> outputPin);

        void callFunction(ScriptStateThread& thread);
        void returnFromFunction(ScriptStateThread& thread, uint8_t outputPins, Vector<ScriptStateThread>& pendingThreads);
        
        void processMessages(Time time, Vector<ScriptStateThread>& pending);

    	EntityId getEntityIdFromUUID(const UUID& uuid) const override;
        UUID getUUIDFromEntityId(EntityId id) const override;
    };
}
