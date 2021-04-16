#include "scripting/script_graph.h"
#include "halley/utils/hash.h"
using namespace Halley;

ScriptGraphNode::ScriptGraphNode()
{}

ScriptGraphNode::ScriptGraphNode(String type, Vector2f position)
	: position(position)
	, type(std::move(type))
{
}

ScriptGraphNode::ScriptGraphNode(const ConfigNode& node)
{
	position = node["position"].asVector2f();
	type = node["type"].asString();
	settings = ConfigNode(node["settings"]);
}

ConfigNode ScriptGraphNode::toConfigNode() const
{
	ConfigNode::MapType result;
	result["position"] = position;
	result["type"] = type;
	result["settings"] = ConfigNode(settings);
	return result;
}

void ScriptGraphNode::setOutput(uint8_t outputPinN, uint32_t targetNode, uint8_t inputPinN)
{
	output.resize(std::max(output.size(), static_cast<size_t>(outputPinN + 1)));
	output[outputPinN] = targetNode;
}

void ScriptGraphNode::feedToHash(Hash::Hasher& hasher)
{
	// TODO
}

ScriptGraph::ScriptGraph()
{
	// Build test graph
	nodes.emplace_back("start", Vector2f(-100, -100)).setOutput(0, 1, 0);
	nodes.emplace_back("playAnimation", Vector2f(0, -100)).setOutput(0, 2, 0);
	nodes.emplace_back("playSound", Vector2f(100, -100));

	computeHash();
}

ScriptGraph::ScriptGraph(const ConfigNode& node)
{
	nodes = node["nodes"].asVector<ScriptGraphNode>();

	computeHash();
}

ConfigNode ScriptGraph::toConfigNode() const
{
	ConfigNode::MapType result;
	result["nodes"] = nodes;
	return result;
}

OptionalLite<uint32_t> ScriptGraph::getStartNode() const
{
	const auto iter = std::find_if(nodes.begin(), nodes.end(), [&] (const ScriptGraphNode& node) { return node.getType() == "start"; });
	if (iter != nodes.end()) {
		return static_cast<uint32_t>(iter - nodes.begin());
	}
	return {};
}

uint64_t ScriptGraph::getHash() const
{
	return hash;
}

void ScriptGraph::computeHash()
{
	Hash::Hasher hasher;
	for (auto& node: nodes) {
		node.feedToHash(hasher);
	}
	hash = hasher.digest();
}

ConfigNode ConfigNodeSerializer<ScriptGraph>::serialize(const ScriptGraph& graph, const ConfigNodeSerializationContext& context)
{
	return graph.toConfigNode();
}

ScriptGraph ConfigNodeSerializer<ScriptGraph>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptGraph(node);
}
