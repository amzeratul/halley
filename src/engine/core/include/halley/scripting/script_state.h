#pragma once
#include "script_message.h"
#include "script_node_enums.h"
#include "script_variables.h"
#include "halley/bytes/config_node_serializer.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class ScriptGraphNode;
	class ScriptGraph;

	class IScriptStateData {
	public:
		virtual ~IScriptStateData() = default;

		virtual ConfigNode toConfigNode(const EntitySerializationContext& context) = 0;
		[[nodiscard]] virtual std::unique_ptr<IScriptStateData> clone() const = 0;
		virtual void copyFrom(IScriptStateData&& src) = 0;
		virtual void finishData() {}
	};

	template <typename T>
	class ScriptStateData: public IScriptStateData {
	public:
		[[nodiscard]] std::unique_ptr<IScriptStateData> clone() const override
		{
			return std::make_unique<T>(dynamic_cast<const T&>(*this));
		}

		void copyFrom(IScriptStateData&& src) override
		{
			dynamic_cast<T&>(*this) = dynamic_cast<T&&>(src);
		}
	};

	class ScriptStateThread {
	public:
		struct StackFrame {
			GraphNodeId node;
			GraphPinId outputPin;
			GraphPinId inputPin;

			StackFrame() = default;
			StackFrame(const ConfigNode& node);
			StackFrame(GraphNodeId node, GraphPinId outputPin, GraphPinId inputPin);
			ConfigNode toConfigNode() const;
			String toString() const;

			bool operator==(const StackFrame& other) const;
			bool operator!=(const StackFrame& other) const;
		};

		ScriptStateThread();
		ScriptStateThread(const ConfigNode& node, const EntitySerializationContext& context);
		ScriptStateThread(GraphNodeId startNode, GraphPinId inputPin);
		ScriptStateThread(const ScriptStateThread& other) = default;
		ScriptStateThread(ScriptStateThread&& other) = default;
		
		ConfigNode toConfigNode(const EntitySerializationContext& context) const;

		ScriptStateThread& operator=(const ScriptStateThread& other) = default;
		ScriptStateThread& operator=(ScriptStateThread&& other) = default;
		
		OptionalLite<GraphNodeId> getCurNode() const { return curNode; }
		GraphPinId getCurInputPin() const { return curInputPin; }

		void advanceToNode(OptionalLite<GraphNodeId> node, GraphPinId outputPin, GraphPinId inputPin);
		ScriptStateThread fork(OptionalLite<GraphNodeId> node, GraphPinId outputPin, GraphPinId inputPin) const;
		
		float& getTimeSlice() { return timeSlice; }
		float& getCurNodeTime() { return curNodeTime; }
		float getCurNodeTime() const { return curNodeTime; }

		bool isRunning() const;
		bool isMerging() const { return merging; }
		void setMerging(bool merging) { this->merging = merging; }
		bool isWatcher() const { return watcher; }
		void setWatcher(bool watcher) { this->watcher = watcher; }

		void merge(const ScriptStateThread& other);

		const Vector<StackFrame>& getStack() const;
		Vector<StackFrame>& getStack();
		bool stackGoesThrough(GraphNodeId node, std::optional<GraphPinId> pin) const;

		uint32_t getUniqueId() const;

		void offsetToNodeRange(Range<GraphNodeId> range);

	private:
		uint32_t uniqueId;
		OptionalLite<GraphNodeId> curNode;
		GraphPinId curInputPin = 0;
		bool merging : 1;
		bool watcher : 1;
		float timeSlice = 0;
		float curNodeTime = 0;
		Vector<StackFrame> stack;

		void generateId();
	};

    class ScriptState {
    public:
		enum class NodeIntrospectionState {
			Unvisited,
			Active,
			Visited
		};
    	
    	struct NodeIntrospection {
    		float time = 0;
    		float activationTime = 0;
    		NodeIntrospectionState state;
    	};

		struct NodeState {
			uint8_t threadCount = 0;
			uint8_t watcherCount = 0;
			bool hasPendingData = false;
			float timeSinceStart = std::numeric_limits<float>::infinity();
			union {
				gsl::owner<IScriptStateData*> data;
				gsl::owner<ConfigNode*> pendingData;
			};

			NodeState();
			NodeState(const ConfigNode& node, const EntitySerializationContext& context);
			NodeState(const NodeState& other);
			NodeState(NodeState&& other);
			~NodeState();

			NodeState& operator=(const NodeState& other);
			NodeState& operator=(NodeState&& other);

			ConfigNode toConfigNode(const EntitySerializationContext& context) const;

			void releaseData();
		};

		enum class ControlEventType: uint8_t {
			StartThread,
			CancelThread,
			NotifyReturn
		};

		struct ControlEvent {
			ControlEventType type;
			GraphNodeId nodeId;
			ConfigNode params;
		};

    	ScriptState();
		ScriptState(const ConfigNode& node, const EntitySerializationContext& context);
		ScriptState(const ScriptGraph* script, bool persistAfterDone);
		ScriptState(std::shared_ptr<const ScriptGraph> script);

		void load(const ConfigNode& node, const EntitySerializationContext& context);
		ConfigNode toConfigNode(const EntitySerializationContext& context) const;
        uint64_t getGraphHash() const { return graphHash; }

		String getScriptId() const;
		const ScriptGraph* getScriptGraphPtr() const;
		void setScriptGraphPtr(const ScriptGraph* script);

		void setTags(Vector<String> tags);
		bool hasTag(const String& tag) const;

		void setStartParams(Vector<ConfigNode> params);
		gsl::span<const ConfigNode> getStartParams() const;

		bool hasStarted() const { return started; }
		bool isDone() const;
		bool isDead() const;

		void setFrameFlag(bool flag);
		bool getFrameFlag() const;

    	void start(uint64_t graphHash);
		void reset();
		void prepareStates(const EntitySerializationContext& context, Time t);

    	NodeState& getNodeState(GraphNodeId nodeId);
		void startNode(const ScriptGraphNode& node, NodeState& state);
		void finishNode(const ScriptGraphNode& node, NodeState& state, bool allThreadsDone);
    	
    	Vector<ScriptStateThread>& getThreads() { return threads; }

    	bool hasThreadAt(GraphNodeId node) const;

    	NodeIntrospection getNodeIntrospection(GraphNodeId nodeId) const;
    	size_t& getNodeCounter(GraphNodeId node);

		void updateDisplayOffset(Time t);
		Vector2f getDisplayOffset() const;

		bool operator==(const ScriptState& other) const;
		bool operator!=(const ScriptState& other) const;

    	int getCurrentFrameNumber() const;
		void incrementFrameNumber();

    	void receiveMessage(ScriptMessage msg);
        void processMessages(Vector<GraphNodeId>& threadsToStart);

    	void receiveControlEvent(ControlEvent event);
		Vector<ControlEvent> processControlEvents();

		ScriptVariables& getLocalVariables();
		const ScriptVariables& getLocalVariables() const;
    	ScriptVariables& getSharedVariables();
		const ScriptVariables& getSharedVariables() const;

		void offsetToNodeRange(Range<GraphNodeId> nodeRange);

	private:
		std::shared_ptr<const ScriptGraph> scriptGraph;
		const ScriptGraph* scriptGraphRef = nullptr;

    	Vector<ScriptStateThread> threads;
		Vector<NodeState> nodeState;
    	std::map<GraphNodeId, size_t> nodeCounters;
    	ScriptVariables localVars;
		ScriptVariables sharedVars;

    	uint64_t graphHash = 0;
		int frameNumber = 0;
    	bool started = false;
		bool persistAfterDone = false;
		bool needsStateLoading = false;
		bool frameFlag = false;

		Vector2f displayOffset;

		Vector<String> tags;
		Vector<ScriptMessage> messageInbox;
		Vector<ControlEvent> controlEventInbox;
		Vector<ConfigNode> startParams;

    	void onNodeStartedIntrospection(GraphNodeId nodeId);
    	void onNodeEndedIntrospection(GraphNodeId nodeId);
		void ensureNodeLoaded(const ScriptGraphNode& node, NodeState& state, const EntitySerializationContext& context);
        bool processMessage(ScriptMessage& msg, Vector<GraphNodeId>& threadsToStart);
    };
	
	template<>
	class ConfigNodeSerializer<ScriptState> {
	public:
		ConfigNode serialize(const ScriptState& state, const EntitySerializationContext& context);
		ScriptState deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptState& target);
	};
	
	template<>
	class ConfigNodeSerializer<ScriptStateThread> {
	public:
		ConfigNode serialize(const ScriptStateThread& thread, const EntitySerializationContext& context);
		ScriptStateThread deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
	
	template<>
	class ConfigNodeSerializer<ScriptState::NodeState> {
	public:
		ConfigNode serialize(const ScriptState::NodeState& state, const EntitySerializationContext& context);
		ScriptState::NodeState deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};

}
