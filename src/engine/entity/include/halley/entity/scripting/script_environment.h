#pragma once
#include "script_node_type.h"

namespace Halley {
    class ScriptState;
	
    class ScriptEnvironment {
    public:
    	ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources, const ScriptNodeTypeCollection& nodeTypeCollection);

    	void update(Time time, const ScriptGraph& graph, ScriptState& state);

    	EntityRef getEntity(EntityId entityId);

    private:
		const HalleyAPI& api;
    	World& world;
    	Resources& resources;
    	const ScriptNodeTypeCollection& nodeTypeCollection;
    	
        IScriptNodeType::Result updateNode(Time time, const ScriptGraphNode& node, IScriptStateData* curData);
        std::unique_ptr<IScriptStateData> makeNodeData(const ScriptGraphNode& node);
    };
}
