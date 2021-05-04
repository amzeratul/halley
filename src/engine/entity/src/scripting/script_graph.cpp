#include "scripting/script_graph.h"

#include "entity.h"
#include "world.h"
#include "halley/utils/hash.h"
using namespace Halley;

ScriptGraphNode::Output::Output(const ConfigNode& node)
{
	if (node.hasKey("nodeId")) {
		nodeId = static_cast<uint32_t>(node["nodeId"].asInt());
	}
	inputPin = static_cast<uint8_t>(node["inputPin"].asInt(0));
}

ConfigNode ScriptGraphNode::Output::toConfigNode() const
{
	ConfigNode::MapType result;
	result["nodeId"] = nodeId ? ConfigNode(static_cast<int>(nodeId.value())) : ConfigNode();
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
	, settings(ConfigNode::MapType())
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

bool ScriptGraphNode::setOutput(uint8_t outputPinN, OptionalLite<uint32_t> targetNode, uint8_t inputPinN)
{
	outputs.resize(std::max(outputs.size(), static_cast<size_t>(outputPinN + 1)));
	
	auto& output = outputs[outputPinN];
	if (output.nodeId != targetNode || output.inputPin != inputPinN) {
		output.nodeId = targetNode;
		output.inputPin = inputPinN;
		return true;
	}

	return false;
}

bool ScriptGraphNode::setTarget(uint8_t targetPinN, EntityId targetEntity)
{
	targets.resize(std::max(targets.size(), static_cast<size_t>(targetPinN + 1)));

	if (targets[targetPinN] != targetEntity) {
		targets[targetPinN] = targetEntity;
		return true;
	}

	return false;
}

void ScriptGraphNode::feedToHash(Hash::Hasher& hasher)
{
	// TODO
}

void ScriptGraphNode::onNodeRemoved(uint32_t nodeId)
{
	disconnectOutputsTo(nodeId, {});
	
	for (auto& o: outputs) {
		if (o.nodeId && o.nodeId.value() >= nodeId) {
			--o.nodeId.value();
		}
	}
}

bool ScriptGraphNode::disconnectOutputsTo(uint32_t nodeId, OptionalLite<uint8_t> pinId)
{
	const size_t startN = outputs.size();
	
	outputs.erase(std::remove_if(outputs.begin(), outputs.end(), [&] (const Output& o)
	{
		return o.nodeId == nodeId && (!pinId || pinId == o.inputPin);
	}), outputs.end());
	
	return startN != outputs.size();
}

String ScriptGraphNode::getTargetName(const World& world, uint8_t idx) const
{
	const EntityId targetId = static_cast<size_t>(idx) < targets.size() ? targets[idx] : EntityId();
	if (targetId.isValid()) {
		const ConstEntityRef target = world.tryGetEntity(targetId);
		if (target.isValid()) {
			return target.getName();
		}
	}
	
	return "<unknown>";
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
	nodes.emplace_back("start", Vector2f(0, -30));
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

ConfigNode ConfigNodeSerializer<ScriptGraphNode>::serialize(const ScriptGraphNode& node, const ConfigNodeSerializationContext& context)
{
	return node.toConfigNode(context);
}

ScriptGraphNode ConfigNodeSerializer<ScriptGraphNode>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptGraphNode(node, context);
}

ConfigNode ConfigNodeSerializer<ScriptGraph>::serialize(const ScriptGraph& graph, const ConfigNodeSerializationContext& context)
{
	return graph.toConfigNode(context);
}

ScriptGraph ConfigNodeSerializer<ScriptGraph>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptGraph(node, context);
}
