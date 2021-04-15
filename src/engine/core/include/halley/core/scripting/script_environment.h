#pragma once
#include "script_graph.h"
#include "script_state.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class ScriptGraph;
    class ScriptState;

	class IScriptNodeType {
	public:
        struct Result {
	        Time timeElapsed = 0;
        	bool done = true;
        };

		virtual String getName() = 0;
		virtual Result update(Time time, const ConfigNode& settings, IScriptStateData* curData) = 0;
		virtual std::unique_ptr<IScriptStateData> makeData() { return {}; }
	};

    class ScriptEnvironment {
    public:
    	ScriptEnvironment();

    	void update(Time time, const ScriptGraph& graph, ScriptState& state);
    	void addScriptNode(std::unique_ptr<IScriptNodeType> nodeType);

    private:
        IScriptNodeType::Result updateNode(Time time, const ScriptGraphNode& node, IScriptStateData* curData);
        std::unique_ptr<IScriptStateData> makeNodeData(const String& type);

    	std::map<String, std::unique_ptr<IScriptNodeType>> nodeTypes;
    };
}
