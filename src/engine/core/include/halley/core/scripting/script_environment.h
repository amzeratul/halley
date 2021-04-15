#pragma once
#include "script_graph.h"
#include "script_state.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class ScriptGraph;
    class ScriptState;

    class ScriptEnvironment {
    public:
    	ScriptEnvironment();

    	void update(Time time, const ScriptGraph& graph, ScriptState& state);

    private:
        std::pair<Time, bool> updateNode(Time time, const ScriptGraphNode& node, IScriptStateData* curData);
        std::unique_ptr<IScriptStateData> makeNodeData(const String& type);
    };
}
