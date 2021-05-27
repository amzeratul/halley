#pragma once
#include "script_node_type.h"

namespace Halley {
    class ScriptState;
	
    class ScriptEnvironment {
    public:
    	ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources, const ScriptNodeTypeCollection& nodeTypeCollection);
    	virtual ~ScriptEnvironment() = default;

    	virtual void update(Time time, const ScriptGraph& graph, ScriptState& graphState);

    	EntityRef tryGetEntity(EntityId entityId);
    	const ScriptGraph* getCurrentGraph() const;
        size_t& getNodeCounter(uint32_t nodeId);

    	void playMusic(const String& music, float fadeTime);
    	void stopMusic(float fadeTime);

    	virtual ConfigNode getVariable(const String& variable);
    	virtual void setVariable(const String& variable, ConfigNode data);

    	virtual void setDirection(EntityId entityId, const String& direction);

    protected:
		const HalleyAPI& api;
    	World& world;
    	Resources& resources;
    	const ScriptNodeTypeCollection& nodeTypeCollection;
    	const ScriptGraph* currentGraph = nullptr;
    	ScriptState* currentState = nullptr;

    private:
        std::unique_ptr<IScriptStateData> makeNodeData(const IScriptNodeType& nodeType, const ScriptGraphNode& node, const ConfigNode& nodeData);
    };
}
