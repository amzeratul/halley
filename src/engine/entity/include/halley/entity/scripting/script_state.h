#pragma once
#include "script_node_enums.h"
#include "halley/bytes/config_node_serializer.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class ScriptGraph;

	class IScriptStateData {
	public:
		virtual ~IScriptStateData() = default;

		virtual ConfigNode toConfigNode(const EntitySerializationContext& context) = 0;
	};

	class ScriptStateThread {
	public:
		ScriptStateThread();
		ScriptStateThread(const ConfigNode& node, const EntitySerializationContext& context);
		explicit ScriptStateThread(ScriptNodeId startNode);

		ScriptStateThread(const ScriptStateThread& other);
		ScriptStateThread(ScriptStateThread&& other) = default;
		ScriptStateThread& operator=(const ScriptStateThread& other);
		ScriptStateThread& operator=(ScriptStateThread&& other) = default;
		
		OptionalLite<ScriptNodeId> getCurNode() const { return curNode; }
		IScriptStateData* getCurData() { return curData.get(); }
		bool isNodeStarted() const { return nodeStarted; }

		void startNode(std::unique_ptr<IScriptStateData> data);
		void finishNode();
		void advanceToNode(OptionalLite<ScriptNodeId> node);

		float& getTimeSlice() { return timeSlice; }

		ConfigNode toConfigNode(const EntitySerializationContext& context) const;

		bool hasPendingNodeData() const;
		ConfigNode getPendingNodeData();

	private:
		OptionalLite<ScriptNodeId> curNode;
		bool nodeStarted = false;
		float timeSlice = 0;
		std::unique_ptr<IScriptStateData> curData;
		ConfigNode pendingData;
		Vector<ScriptNodeId> stack;
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

    	ScriptState();
		ScriptState(const ConfigNode& node, const EntitySerializationContext& context);
		ScriptState(const ScriptGraph* script, bool persistAfterDone);
		ScriptState(std::shared_ptr<const ScriptGraph> script);

		const ScriptGraph* getScriptGraphPtr() const;
		void setScriptGraphPtr(const ScriptGraph* script);

		bool hasStarted() const { return started; }
		bool isDone() const;
		bool isDead() const;

    	void start(OptionalLite<ScriptNodeId> startNode, uint64_t graphHash);
		void reset();
    	
    	Vector<ScriptStateThread>& getThreads() { return threads; }

		ConfigNode toConfigNode(const EntitySerializationContext& context) const;
        uint64_t getGraphHash() const { return graphHash; }

    	bool hasThreadAt(ScriptNodeId node) const;

    	void setIntrospection(bool enabled);
    	void updateIntrospection(Time t);
        void onNodeStarted(ScriptNodeId nodeId)
        {
	        if (introspection) {
		        onNodeStartedIntrospection(nodeId);
	        }
        }
    	void onNodeEnded(ScriptNodeId nodeId)
    	{
	        if (introspection) {
		        onNodeEndedIntrospection(nodeId);
	        }
        }
    	NodeIntrospection getNodeIntrospection(ScriptNodeId nodeId) const;

    	size_t& getNodeCounter(ScriptNodeId node);

    	ConfigNode getVariable(const String& name) const;
    	void setVariable(const String& name, ConfigNode value);

		bool operator==(const ScriptState& other) const;
		bool operator!=(const ScriptState& other) const;

	private:
		std::shared_ptr<const ScriptGraph> scriptGraph;
		const ScriptGraph* scriptGraphRef = nullptr;
    	Vector<ScriptStateThread> threads;
    	uint64_t graphHash = 0;
    	bool started = false;
    	bool introspection = false;
		bool persistAfterDone = false;
    	std::map<ScriptNodeId, size_t> nodeCounters;
    	std::map<String, ConfigNode> variables;

    	Vector<NodeIntrospection> nodeIntrospection;

    	void onNodeStartedIntrospection(ScriptNodeId nodeId);
    	void onNodeEndedIntrospection(ScriptNodeId nodeId);
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

}
