#include "scripting/script_graph.h"

#include "entity.h"
#include "world.h"
#include "halley/utils/hash.h"
using namespace Halley;

ScriptGraphNode::Pin::Pin(const ConfigNode& node, const ConfigNodeSerializationContext& context)
{
	if (node.hasKey("dstNode")) {
		dstNode = static_cast<uint32_t>(node["dstNode"].asInt());
	}
	if (node.hasKey("entity")) {
		entity = ConfigNodeSerializer<EntityId>().deserialize(context, node["entity"]);
	}
	dstPin = static_cast<uint8_t>(node["dstPin"].asInt(0));
}

ConfigNode ScriptGraphNode::Pin::toConfigNode(const ConfigNodeSerializationContext& context) const
{
	ConfigNode::MapType result;
	if (dstNode) {
		result["dstNode"] = ConfigNode(static_cast<int>(dstNode.value()));
	}
	if (dstPin != 0) {
		result["dstPin"] = static_cast<int>(dstPin);
	}
	if (entity.isValid()) {
		result["entity"] = ConfigNodeSerializer<EntityId>().serialize(entity, context);
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
	pins = ConfigNodeSerializer<std::vector<Pin>>().deserialize(context, node["pins"]);
}

ConfigNode ScriptGraphNode::toConfigNode(const ConfigNodeSerializationContext& context) const
{
	ConfigNode::MapType result;
	result["position"] = position;
	result["type"] = type;
	result["settings"] = ConfigNode(settings);
	result["pins"] = ConfigNodeSerializer<std::vector<Pin>>().serialize(pins, context);
	return result;
}

bool ScriptGraphNode::connectPin(uint8_t pinN, OptionalLite<uint32_t> dstNode, uint8_t dstPinN)
{
	pins.resize(std::max(pins.size(), static_cast<size_t>(pinN + 1)));
	
	auto& pin = pins[pinN];
	if (pin.dstNode != dstNode || pin.dstPin != dstPinN || pin.entity.isValid()) {
		pin.dstNode = dstNode;
		pin.dstPin = dstPinN;
		pin.entity = EntityId();
		return true;
	}

	return false;
}

bool ScriptGraphNode::connectTarget(uint8_t pinN, EntityId targetEntity)
{
	pins.resize(std::max(pins.size(), static_cast<size_t>(pinN + 1)));

	auto& pin = pins[pinN];
	if (pin.entity != targetEntity || pin.dstNode.has_value() || pin.dstPin != 0) {
		pin.entity = targetEntity;
		pin.dstNode = OptionalLite<uint32_t>{};
		pin.dstPin = 0;
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
	disconnectPinsTo(nodeId, {});
	
	for (auto& o: pins) {
		if (o.dstNode && o.dstNode.value() >= nodeId) {
			--o.dstNode.value();
		}
	}
}

bool ScriptGraphNode::disconnectPinsTo(uint32_t nodeId, OptionalLite<uint8_t> pinId)
{
	const size_t startN = pins.size();
	
	pins.erase(std::remove_if(pins.begin(), pins.end(), [&] (const Pin& o)
	{
		return o.dstNode == nodeId && (!pinId || pinId == o.dstPin);
	}), pins.end());
	
	return startN != pins.size();
}

String ScriptGraphNode::getTargetName(const World& world, uint8_t idx) const
{
	const EntityId targetId = static_cast<size_t>(idx) < pins.size() ? pins[idx].entity : EntityId();
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

ConfigNode ConfigNodeSerializer<ScriptGraphNode::Pin>::serialize(const ScriptGraphNode::Pin& pin, const ConfigNodeSerializationContext& context)
{
	return pin.toConfigNode(context);
}

ScriptGraphNode::Pin ConfigNodeSerializer<ScriptGraphNode::Pin>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptGraphNode::Pin(node, context);
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
