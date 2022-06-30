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
			ScriptNodeId node;
			ScriptPinId pin;

			StackFrame() = default;
			StackFrame(const ConfigNode& node);
			StackFrame(ScriptNodeId node, ScriptPinId pin);
			ConfigNode toConfigNode() const;

			bool operator==(const StackFrame& other) const;
			bool operator!=(const StackFrame& other) const;
		};

		ScriptStateThread();
		ScriptStateThread(const ConfigNode& node, const EntitySerializationContext& context);
		ScriptStateThread(ScriptNodeId startNode);
		ScriptStateThread(const ScriptStateThread& other) = default;
		ScriptStateThread(ScriptStateThread&& other) = default;
		
		ConfigNode toConfigNode(const EntitySerializationContext& context) const;

		ScriptStateThread& operator=(const ScriptStateThread& other) = default;
		ScriptStateThread& operator=(ScriptStateThread&& other) = default;
		
		OptionalLite<ScriptNodeId> getCurNode() const { return curNode; }

		void advanceToNode(OptionalLite<ScriptNodeId> node, ScriptPinId outputPin);
		ScriptStateThread fork(OptionalLite<ScriptNodeId> node, ScriptPinId outputPin) const;
		
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
		bool stackGoesThrough(ScriptNodeId node, std::optional<ScriptPinId> pin) const;

	private:
		OptionalLite<ScriptNodeId> curNode;
		bool merging = false;
		bool watcher = false;
		float timeSlice = 0;
		float curNodeTime = 0;
		Vector<StackFrame> stack;
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
			bool hasPendingData = false;
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

    	ScriptState();
		ScriptState(const ConfigNode& node, const EntitySerializationContext& context);
		ScriptState(const ScriptGraph* script, bool persistAfterDone);
		ScriptState(std::shared_ptr<const ScriptGraph> script);

		const String& getScriptId() const;
		const ScriptGraph* getScriptGraphPtr() const;
		void setScriptGraphPtr(const ScriptGraph* script);

		void setTags(Vector<String> tags);
		bool hasTag(const String& tag) const;
		
		bool hasStarted() const { return started; }
		bool isDone() const;
		bool isDead() const;

    	void start(OptionalLite<ScriptNodeId> startNode, uint64_t graphHash);
		void reset();
		void ensureReady(const EntitySerializationContext& context);

    	NodeState& getNodeState(ScriptNodeId nodeId);
		void startNode(const ScriptGraphNode& node, NodeState& state);
		void finishNode(const ScriptGraphNode& node, NodeState& state, bool allThreadsDone);
    	
    	Vector<ScriptStateThread>& getThreads() { return threads; }

		ConfigNode toConfigNode(const EntitySerializationContext& context) const;
        uint64_t getGraphHash() const { return graphHash; }

    	bool hasThreadAt(ScriptNodeId node) const;

    	NodeIntrospection getNodeIntrospection(ScriptNodeId nodeId) const;
    	size_t& getNodeCounter(ScriptNodeId node);

		void updateDisplayOffset(Time t);
		Vector2f getDisplayOffset() const;

		bool operator==(const ScriptState& other) const;
		bool operator!=(const ScriptState& other) const;

    	int getCurrentFrameNumber() const;
		void incrementFrameNumber();

    	void receiveMessage(ScriptMessage msg);
        void processMessages();

		ScriptVariables& getLocalVariables();
		const ScriptVariables& getLocalVariables() const;
    	ScriptVariables& getSharedVariables();
		const ScriptVariables& getSharedVariables() const;

	private:
		std::shared_ptr<const ScriptGraph> scriptGraph;
		const ScriptGraph* scriptGraphRef = nullptr;

    	Vector<ScriptStateThread> threads;
		Vector<NodeState> nodeState;
    	std::map<ScriptNodeId, size_t> nodeCounters;
    	ScriptVariables localVars;
		ScriptVariables sharedVars;

    	uint64_t graphHash = 0;
		int frameNumber = 0;
    	bool started = false;
		bool persistAfterDone = false;
		bool needsStateLoading = false;

		Vector2f displayOffset;

		Vector<String> tags;
		Vector<ScriptMessage> inbox;

    	void onNodeStartedIntrospection(ScriptNodeId nodeId);
    	void onNodeEndedIntrospection(ScriptNodeId nodeId);
		void ensureNodeLoaded(const ScriptGraphNode& node, NodeState& state, const EntitySerializationContext& context);
        bool processMessage(ScriptMessage& msg);
    };
	
	template<>
	class ConfigNodeSerializer<ScriptState> {
	public:
		ConfigNode serialize(const ScriptState& state, const EntitySerializationContext& context);
		ScriptState deserialize(const EntitySerializationContext& context, const ConfigNode& node);
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
