#pragma once
#include "script_message.h"
#include "../entity_factory.h"
#include "script_node_type.h"
#include "halley/core/input/input_virtual.h"

namespace Halley {
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
        };

    	ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources, const ScriptNodeTypeCollection& nodeTypeCollection);
    	virtual ~ScriptEnvironment() = default;

    	virtual void update(Time time, ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables);
    	void terminateState(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables);
		ConfigNode readNodeElementDevConData(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables, ScriptNodeId nodeId, ScriptPinId pinId);

    	EntityRef tryGetEntity(EntityId entityId);
    	const ScriptGraph* getCurrentGraph() const;
        size_t& getNodeCounter(ScriptNodeId nodeId);
        IScriptStateData* getNodeData(ScriptNodeId nodeId);
        void assignTypes(const ScriptGraph& graph);

    	ConfigNode readInputDataPin(const ScriptGraphNode& node, ScriptPinId pinN);
        ConfigNode readOutputDataPin(const ScriptGraphNode& node, ScriptPinId pinN);
        EntityId readInputEntityIdRaw(const ScriptGraphNode& node, ScriptPinId pinN);
        EntityId readInputEntityId(const ScriptGraphNode& node, ScriptPinId pinN);
        EntityId readOutputEntityId(const ScriptGraphNode& node, ScriptPinId pinN);

    	void postAudioEvent(const String& id, EntityId entityId);

        ScriptVariables& getVariables(ScriptVariableScope scope);
        const ScriptVariables& getVariables(ScriptVariableScope scope) const;

    	virtual void setDirection(EntityId entityId, const String& direction);

        void setInputEnabled(bool enabled);
        virtual bool isInputEnabled() const;
        void setInputDevice(EntityId target, std::shared_ptr<InputVirtual> input);
        std::shared_ptr<InputVirtual> getInputDevice(EntityId target) const;
        virtual int getInputButtonByName(const String& name) const;

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
        ScriptPinId getCurrentInputPin() const;

        World& getWorld();
        Resources& getResources();

    	void sendScriptMessage(EntityId dstEntity, ScriptMessage message);
        void sendEntityMessage(EntityMessageData message);
        void sendSystemMessage(SystemMessageData message);

        void startScript(EntityId target, const String& scriptName, Vector<String> tags);
        void stopScript(EntityId target, const String& scriptName);
        void stopScriptTag(EntityId target, const String& tag);

    	Vector<std::pair<EntityId, ScriptMessage>> getOutboundScriptMessages();
        Vector<EntityMessageData> getOutboundEntityMessages();
        Vector<ScriptExecutionRequest> getScriptExecutionRequests();

        virtual std::shared_ptr<UIWidget> createInWorldUI(const String& ui, Vector2f offset, Vector2f alignment, EntityId entityId);
        virtual std::shared_ptr<UIWidget> createModalUI(const String& ui, ConfigNode data);

    protected:
		const HalleyAPI& api;
    	World& world;
    	Resources& resources;
        HashMap<EntityId, std::shared_ptr<InputVirtual>> inputDevices;
    	const ScriptNodeTypeCollection& nodeTypeCollection;
        bool isHost = false;
        bool inputEnabled = true;

        ScriptPinId currentInputPin = 0;
    	const ScriptGraph* currentGraph = nullptr;
    	ScriptState* currentState = nullptr;
        ScriptVariables* currentEntityVariables = nullptr;
        EntityId currentEntity;
        Time deltaTime = 0;
        EntitySerializationContext serializationContext;

        Vector<std::pair<EntityId, ScriptMessage>> scriptOutbox;
        Vector<EntityMessageData> entityOutbox;
        Vector<ScriptExecutionRequest> scriptExecutionRequestOutbox;

    private:
        void updateThread(ScriptState& graphState, ScriptStateThread& thread, Vector<ScriptStateThread>& pendingThreads);
        void doTerminateState();
        void runDestructor(ScriptNodeId nodeId);

        ScriptStateThread startThread(ScriptStateThread thread);
        void addThread(ScriptStateThread thread, Vector<ScriptStateThread>& pending);
        void advanceThread(ScriptStateThread& thread, OptionalLite<ScriptNodeId> node, ScriptPinId outputPin, ScriptPinId inputPin);
        void initNode(ScriptNodeId nodeId, ScriptState::NodeState& state);
        void forkThread(ScriptStateThread& thread, std::array<IScriptNodeType::OutputNode, 8> outputNodes, Vector<ScriptStateThread>& pendingThreads, size_t firstIdx = 0);
        void mergeThread(ScriptStateThread& thread, bool wait);
        void terminateThread(ScriptStateThread& thread, bool allowRollback);
        void removeStoppedThreads();

        void cancelOutputs(ScriptNodeId nodeId, uint8_t cancelMask);
        void abortCodePath(ScriptNodeId node, std::optional<ScriptPinId> outputPin);

        void callFunction(ScriptStateThread& thread);
        void returnFromFunction(ScriptStateThread& thread, uint8_t outputPins);
        
        void processMessages(Time time, Vector<ScriptStateThread>& pending);

    	EntityId getEntityIdFromUUID(const UUID& uuid) const override;
        UUID getUUIDFromEntityId(EntityId id) const override;
    };
}
