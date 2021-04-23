#pragma once
#include "halley/bytes/config_node_serializer.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class ScriptGraph;

	class IScriptStateData {
	public:
		virtual ~IScriptStateData() = default;
	};

	class ScriptStateThread {
	public:
		ScriptStateThread();
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

		Time& getTimeSlice() { return timeSlice; }

	private:
		OptionalLite<uint32_t> curNode;
		bool nodeStarted = false;
		std::unique_ptr<IScriptStateData> curData;
		Time timeSlice;
	};

    class ScriptState {
    public:
    	ScriptState();
		ScriptState(const ConfigNode& node);

    	bool hasStarted() const { return started; }
    	void start(OptionalLite<uint32_t> startNode, uint64_t graphHash);
    	
    	std::vector<ScriptStateThread>& getThreads() { return threads; }

		ConfigNode toConfigNode() const;
        uint64_t getGraphHash() const { return graphHash; }

    	bool hasThreadAt(uint32_t node) const;

    private:
    	std::vector<ScriptStateThread> threads;
    	uint64_t graphHash;
    	bool started;
    };
	
	template<>
	class ConfigNodeSerializer<ScriptState> {
	public:
		ConfigNode serialize(const ScriptState& state, const ConfigNodeSerializationContext& context);
		ScriptState deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node);
	};

}
