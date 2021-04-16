#include "scripting/script_graph.h"
#include "halley/utils/hash.h"
using namespace Halley;

ScriptGraphNode::Output::Output(const ConfigNode& node)
{
	nodeId = static_cast<uint32_t>(node["nodeId"].asInt());
	inputPin = static_cast<uint8_t>(node["inputPin"].asInt(0));
}

ConfigNode ScriptGraphNode::Output::toConfigNode() const
{
	ConfigNode::MapType result;
	result["nodeId"] = static_cast<int>(nodeId);
	if (inputPin != 0) {
		result["inputPin"] = static_cast<int>(inputPin);
	}
	return result;
}

ScriptGraphNode::ScriptGraphNode()
{}

ScriptGraphNode::ScriptGraphNode(String type, Vector2f position)
	: position(position)
	, type(std::move(type))
{
}

ScriptGraphNode::ScriptGraphNode(const ConfigNode& node, const ConfigNodeSerializationContext& context)
{
	position = node["position"].asVector2f();
	type = node["type"].asString();
	settings = ConfigNode(node["settings"]);
	outputs = node["outputs"].asVector<Output>();
	targets = ConfigNodeSerializer<std::vector<EntityId>>().deserialize(context, node["targets"]);
	/*
	if (node["targets"].getType() == ConfigNodeType::Sequence) {
		const auto& seq = node["targets"].asSequence();
		targets.reserve(seq.size());
		for (const auto& n: seq) {
			targets.push_back(ConfigNodeSerializer<EntityId>().deserialize(context, n));
		}
	}
	*/
}

ConfigNode ScriptGraphNode::toConfigNode(const ConfigNodeSerializationContext& context) const
{
	ConfigNode::MapType result;
	result["position"] = position;
	result["type"] = type;
	result["settings"] = ConfigNode(settings);
	result["outputs"] = outputs;
	result["targets"] = ConfigNodeSerializer<std::vector<EntityId>>().serialize(targets, context);
	/*
	ConfigNode::SequenceType targetNodes;
	targetNodes.reserve(targets.size());
	for (const auto& target: targets) {
		targetNodes.push_back(ConfigNodeSerializer<EntityId>().serialize(target, context));
	}
	result["targets"] = std::move(targetNodes);
	*/
	return result;
}

void ScriptGraphNode::setOutput(uint8_t outputPinN, uint32_t targetNode, uint8_t inputPinN)
{
	outputs.resize(std::max(outputs.size(), static_cast<size_t>(outputPinN + 1)));
	auto& output = outputs[outputPinN];
	output.nodeId = targetNode;
	output.inputPin = inputPinN;
}

void ScriptGraphNode::feedToHash(Hash::Hasher& hasher)
{
	// TODO
}

ScriptGraph::ScriptGraph()
{
	makeBaseGraph();
	computeHash();
}

ScriptGraph::ScriptGraph(const ConfigNode& node, const ConfigNodeSerializationContext& context)
{
	nodes = ConfigNodeSerializer<std::vector<ScriptGraphNode>>().deserialize(context, node["nodes"]);
	if (nodes.empty()) {
		makeBaseGraph();
	}
	computeHash();
}

ConfigNode ScriptGraph::toConfigNode(const ConfigNodeSerializationContext& context) const
{
	ConfigNode::MapType result;
	result["nodes"] = ConfigNodeSerializer<std::vector<ScriptGraphNode>>().serialize(nodes, context);
	return result;
}

void ScriptGraph::makeBaseGraph()
{
	nodes.emplace_back("start", Vector2f(0, -100));
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
	return graph.toConfigNode(context);
}

ScriptGraph ConfigNodeSerializer<ScriptGraph>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptGraph(node, context);
}
