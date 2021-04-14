#include "halley/core/scripting/script_graph.h"
using namespace Halley;

ScriptGraphNode::ScriptGraphNode()
{}

ScriptGraphNode::ScriptGraphNode(const ConfigNode& node)
{
	position = node["position"].asVector2f();
}

ConfigNode ScriptGraphNode::toConfigNode() const
{
	ConfigNode::MapType result;
	result["position"] = position;
	return result;
}

ScriptGraph::ScriptGraph()
{}

ScriptGraph::ScriptGraph(const ConfigNode& node)
{
	nodes = node["nodes"].asVector<ScriptGraphNode>();
}

ConfigNode ScriptGraph::toConfigNode() const
{
	ConfigNode::MapType result;
	result["nodes"] = nodes;
	return result;
}

ConfigNode ConfigNodeSerializer<ScriptGraph>::serialize(const ScriptGraph& graph, const ConfigNodeSerializationContext& context)
{
	return graph.toConfigNode();
}

ScriptGraph ConfigNodeSerializer<ScriptGraph>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptGraph(node);
}
