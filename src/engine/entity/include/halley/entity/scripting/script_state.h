#pragma once
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
		explicit ScriptStateThread(uint32_t startNode);

		ScriptStateThread(const ScriptStateThread& other);
		ScriptStateThread(ScriptStateThread&& other) = default;
		ScriptStateThread& operator=(const ScriptStateThread& other);
		ScriptStateThread& operator=(ScriptStateThread&& other) = default;
		
		OptionalLite<uint32_t> getCurNode() const { return curNode; }
		IScriptStateData* getCurData() { return curData.get(); }
		bool isNodeStarted() const { return nodeStarted; }

		void startNode(std::unique_ptr<IScriptStateData> data);
		void finishNode();
		void advanceToNode(OptionalLite<uint32_t> node);

		float& getTimeSlice() { return timeSlice; }

		ConfigNode toConfigNode(const EntitySerializationContext& context) const;

		bool hasPendingNodeData() const;
		ConfigNode getPendingNodeData();

	private:
		OptionalLite<uint32_t> curNode;
		bool nodeStarted = false;
		std::unique_ptr<IScriptStateData> curData;
		float timeSlice;
		ConfigNode pendingData;
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

    	bool hasStarted() const { return started; }
    	void start(OptionalLite<uint32_t> startNode, uint64_t graphHash);
		void reset();
    	
    	std::vector<ScriptStateThread>& getThreads() { return threads; }

		ConfigNode toConfigNode(const EntitySerializationContext& context) const;
        uint64_t getGraphHash() const { return graphHash; }

    	bool hasThreadAt(uint32_t node) const;

    	void setIntrospection(bool enabled);
    	void updateIntrospection(Time t);
        void onNodeStarted(uint32_t nodeId)
        {
	        if (introspection) {
		        onNodeStartedIntrospection(nodeId);
	        }
        }
    	void onNodeEnded(uint32_t nodeId)
    	{
	        if (introspection) {
		        onNodeEndedIntrospection(nodeId);
	        }
        }
    	NodeIntrospection getNodeIntrospection(uint32_t nodeId) const;

    	size_t& getNodeCounter(uint32_t node);

    	ConfigNode getVariable(const String& name) const;
    	void setVariable(const String& name, ConfigNode value);

	private:
    	std::vector<ScriptStateThread> threads;
    	uint64_t graphHash = 0;
    	bool started = false;
    	bool introspection = false;
    	std::map<uint32_t, size_t> nodeCounters;
    	std::map<String, ConfigNode> variables;

    	std::vector<NodeIntrospection> nodeIntrospection;

    	void onNodeStartedIntrospection(uint32_t nodeId);
    	void onNodeEndedIntrospection(uint32_t nodeId);
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
