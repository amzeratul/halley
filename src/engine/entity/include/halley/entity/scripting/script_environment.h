#pragma once
#include "script_message.h"
#include "../entity_factory.h"
#include "script_node_type.h"

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

    	ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources, const ScriptNodeTypeCollection& nodeTypeCollection);
    	virtual ~ScriptEnvironment() = default;

    	virtual void update(Time time, ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables);
    	void terminateState(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables);

    	EntityRef tryGetEntity(EntityId entityId);
    	const ScriptGraph* getCurrentGraph() const;
        size_t& getNodeCounter(ScriptNodeId nodeId);
        IScriptStateData* getNodeData(ScriptNodeId nodeId);
        void assignTypes(const ScriptGraph& graph);

    	void postAudioEvent(const String& id, EntityId entityId);

        ScriptVariables& getVariables(ScriptVariableScope scope);
        const ScriptVariables& getVariables(ScriptVariableScope scope) const;

    	virtual void setDirection(EntityId entityId, const String& direction);

        void setInputDevice(int idx, std::shared_ptr<InputDevice> input);
        std::shared_ptr<InputDevice> getInputDevice(int idx) const;
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

        World& getWorld();
        Resources& getResources();

    	void sendScriptMessage(EntityId dstEntity, ScriptMessage message);
        void sendEntityMessage(EntityMessageData message);
        void sendSystemMessage(SystemMessageData message);

    	Vector<std::pair<EntityId, ScriptMessage>> getOutboundScriptMessages();
        Vector<EntityMessageData> getOutboundEntityMessages();

        virtual std::shared_ptr<UIWidget> createInWorldUI(const String& ui, Vector2f offset, Vector2f alignment, EntityId entityId);
        virtual std::shared_ptr<UIWidget> createModalUI(const String& ui);

    protected:
		const HalleyAPI& api;
    	World& world;
    	Resources& resources;
        HashMap<int, std::shared_ptr<InputDevice>> inputDevices;
    	const ScriptNodeTypeCollection& nodeTypeCollection;
        bool isHost = false;

    	const ScriptGraph* currentGraph = nullptr;
    	ScriptState* currentState = nullptr;
        ScriptVariables* currentEntityVariables = nullptr;
        EntityId currentEntity;
        Time deltaTime = 0;
        EntitySerializationContext serializationContext;

        Vector<std::pair<EntityId, ScriptMessage>> scriptOutbox;
        Vector<EntityMessageData> entityOutbox;

    private:
        void updateThread(ScriptState& graphState, ScriptStateThread& thread, Vector<ScriptStateThread>& pendingThreads);
        void doTerminateState();
        void addThread(ScriptStateThread thread, Vector<ScriptStateThread>& pending);
        void advanceThread(ScriptStateThread& thread, OptionalLite<ScriptNodeId> node, ScriptPinId outputPin);
        void forkThread(ScriptStateThread& thread, std::array<IScriptNodeType::OutputNode, 8> outputNodes, Vector<ScriptStateThread>& pendingThreads, size_t firstIdx = 0);
        void mergeThread(ScriptStateThread& thread, bool wait);
        void terminateThread(ScriptStateThread& thread, bool allowRollback);
        void removeStoppedThreads();

        void cancelOutputs(ScriptNodeId nodeId, uint8_t cancelMask);
        void abortCodePath(ScriptNodeId node, std::optional<ScriptPinId> outputPin);
        
        EntityId getEntityIdFromUUID(const UUID& uuid) const override;
        UUID getUUIDFromEntityId(EntityId id) const override;
    };
}
