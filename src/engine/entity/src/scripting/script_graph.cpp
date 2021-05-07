#include "scripting/script_graph.h"

#include "entity.h"
#include "world.h"
#include "halley/utils/hash.h"
#include "scripting/script_node_type.h"
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

void ScriptGraphNode::assignType(const ScriptNodeTypeCollection& nodeTypeCollection) const
{
	nodeType = nodeTypeCollection.tryGetNodeType(type);
}

const IScriptNodeType& ScriptGraphNode::getNodeType() const
{
	Expects(nodeType != nullptr);
	return *nodeType;
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

bool ScriptGraph::connectPins(uint32_t srcNodeIdx, uint8_t srcPinN, uint32_t dstNodeIdx, uint8_t dstPinN)
{
	auto& srcNode = nodes.at(srcNodeIdx);
	auto& srcPin = srcNode.getPin(srcPinN);
	auto& dstNode = nodes.at(dstNodeIdx);
	auto& dstPin = dstNode.getPin(dstPinN);

	if (srcPin.dstNode == dstNodeIdx && srcPin.dstPin == dstPinN && dstPin.dstNode == srcNodeIdx && dstPin.dstPin == srcPinN) {
		return false;
	}

	disconnectPin(srcNodeIdx, srcPinN);
	disconnectPin(dstNodeIdx, dstPinN);
	
	srcPin.dstNode = dstNodeIdx;
	srcPin.dstPin = dstPinN;
	dstPin.dstNode = srcNodeIdx;
	dstPin.dstPin = srcPinN;

	return true;
}

bool ScriptGraph::connectPin(uint32_t srcNodeIdx, uint8_t srcPinN, EntityId target)
{
	auto& srcNode = nodes.at(srcNodeIdx);
	auto& srcPin = srcNode.getPin(srcPinN);

	if (srcPin.entity == target) {
		return false;
	}

	disconnectPin(srcNodeIdx, srcPinN);

	srcPin.entity = target;

	return true;
}

bool ScriptGraph::disconnectPin(uint32_t nodeIdx, uint8_t pinN)
{
	auto& node = nodes.at(nodeIdx);
	auto& pin = node.getPin(pinN);
	if (!pin.dstNode.has_value() && pin.dstPin == 0 && !pin.entity.isValid()) {
		return false;
	}

	if (pin.dstNode) {
		auto& otherNode = nodes.at(pin.dstNode.value());
		auto& otherPin = otherNode.getPin(pin.dstPin);

		otherPin.dstNode = OptionalLite<uint32_t>();
		otherPin.dstPin = 0;
		otherPin.entity = EntityId();
	}

	pin.dstNode = OptionalLite<uint32_t>();
	pin.dstPin = 0;
	pin.entity = EntityId();

	return true;
}

void ScriptGraph::assignTypes(const ScriptNodeTypeCollection& nodeTypeCollection) const
{
	if (lastAssignTypeHash != hash) {
		lastAssignTypeHash = hash;
		for (const auto& node: nodes) {
			node.assignType(nodeTypeCollection);
		}
	}
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
