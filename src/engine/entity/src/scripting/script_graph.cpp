#include "scripting/script_graph.h"

#include "entity.h"
#include "world.h"
#include "halley/utils/hash.h"
#include "scripting/script_node_type.h"
using namespace Halley;

ScriptGraphNode::PinConnection::PinConnection(const ConfigNode& node, const ConfigNodeSerializationContext& context)
{
	if (node.hasKey("dstNode")) {
		dstNode = static_cast<uint32_t>(node["dstNode"].asInt());
	}
	if (node.hasKey("entity")) {
		entity = ConfigNodeSerializer<EntityId>().deserialize(context, node["entity"]);
	}
	dstPin = static_cast<uint8_t>(node["dstPin"].asInt(0));
}

ScriptGraphNode::PinConnection::PinConnection(uint32_t dstNode, uint8_t dstPin)
	: dstNode(dstNode)
	, dstPin(dstPin)
{
}

ScriptGraphNode::PinConnection::PinConnection(EntityId entity)
	: entity(entity)
{
}

ConfigNode ScriptGraphNode::PinConnection::toConfigNode(const ConfigNodeSerializationContext& context) const
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

ScriptGraphNode::Pin::Pin(const ConfigNode& node, const ConfigNodeSerializationContext& context)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		connections = ConfigNodeSerializer<std::vector<PinConnection>>().deserialize(context, node);
	} else if (node.getType() == ConfigNodeType::Map) {
		connections.clear();
		connections.push_back(ConfigNodeSerializer<PinConnection>().deserialize(context, node));
	}
}

ConfigNode ScriptGraphNode::Pin::toConfigNode(const ConfigNodeSerializationContext& context) const
{
	return ConfigNodeSerializer<std::vector<PinConnection>>().serialize(connections, context);
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

EntityId ScriptGraphNode::getEntityAtPin(size_t idx) const
{
	return pins.at(idx).connections.at(0).entity;
}

EntityId ScriptGraphNode::tryGetEntityAtPin(size_t idx) const
{
	if (idx < pins.size()) {
		const auto& pin = pins[idx];
		if (!pin.connections.empty()) {
			return pin.connections[0].entity;
		}
	}
	return EntityId();
}

void ScriptGraphNode::feedToHash(Hash::Hasher& hasher)
{
	// TODO
}

void ScriptGraphNode::onNodeRemoved(uint32_t nodeId)
{
	for (auto& pin: pins) {
		for (auto& o: pin.connections) {
			if (o.dstNode) {
				if (o.dstNode.value() == nodeId) {
					o.dstNode = OptionalLite<uint32_t>();
					o.dstPin = 0;
				} else if (o.dstNode.value() >= nodeId) {
					--o.dstNode.value();
				}
			}
		}
	}
}

String ScriptGraphNode::getTargetName(const World& world, uint8_t idx) const
{
	const EntityId targetId = tryGetEntityAtPin(idx);
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

ScriptNodePinType ScriptGraphNode::getPinType(uint8_t idx) const
{
	const auto& config = getNodeType().getPinConfiguration();
	if (idx >= config.size()) {
		return ScriptNodePinType();
	}
	return config[idx];
}

ScriptGraph::ScriptGraph()
{
	makeBaseGraph();
	finishGraph();
}

ScriptGraph::ScriptGraph(const ConfigNode& node, const ConfigNodeSerializationContext& context)
{
	nodes = ConfigNodeSerializer<std::vector<ScriptGraphNode>>().deserialize(context, node["nodes"]);
	if (nodes.empty()) {
		makeBaseGraph();
	}
	finishGraph();
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

	for (const auto& conn: srcPin.connections) {
		if (conn.dstNode == dstNodeIdx && conn.dstPin == dstPinN) {
			return false;
		}
	}

	disconnectPinIfSingleConnection(srcNodeIdx, srcPinN);
	disconnectPinIfSingleConnection(dstNodeIdx, dstPinN);
	
	srcPin.connections.emplace_back(ScriptGraphNode::PinConnection{ dstNodeIdx, dstPinN });
	dstPin.connections.emplace_back(ScriptGraphNode::PinConnection{ srcNodeIdx, srcPinN });

	return true;
}

bool ScriptGraph::connectPin(uint32_t srcNodeIdx, uint8_t srcPinN, EntityId target)
{
	auto& srcNode = nodes.at(srcNodeIdx);
	auto& srcPin = srcNode.getPin(srcPinN);

	for (const auto& conn: srcPin.connections) {
		if (conn.entity == target) {
			return false;
		}
	}

	disconnectPinIfSingleConnection(srcNodeIdx, srcPinN);

	srcPin.connections.emplace_back(ScriptGraphNode::PinConnection{ target });

	return true;
}

bool ScriptGraph::disconnectPinIfSingleConnection(uint32_t nodeIdx, uint8_t pinN)
{
	auto& node = nodes.at(nodeIdx);
	if (node.getPinType(pinN).isMultiConnection()) {
		return false;
	}

	auto& pin = node.getPin(pinN);
	if (pin.connections.empty()) {
		return false;
	}

	for (auto& conn: pin.connections) {
		if (!conn.dstNode.has_value() && conn.dstPin == 0 && !conn.entity.isValid()) {
			return false;
		}

		if (conn.dstNode) {
			auto& otherNode = nodes.at(conn.dstNode.value());
			auto& ocs = otherNode.getPin(conn.dstPin).connections;
			ocs.erase(std::remove_if(ocs.begin(), ocs.end(), [&] (const auto& oc) { return oc.dstNode == nodeIdx && oc.dstPin == pinN; }), ocs.end());
		}
	}

	pin.connections.clear();

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

void ScriptGraph::finishGraph()
{
	Hash::Hasher hasher;
	uint32_t i = 0;
	for (auto& node: nodes) {
		node.setId(i++);
		node.feedToHash(hasher);
	}
	hash = hasher.digest();
}

ConfigNode ConfigNodeSerializer<ScriptGraphNode::PinConnection>::serialize(const ScriptGraphNode::PinConnection& connection, const ConfigNodeSerializationContext& context)
{
	return connection.toConfigNode(context);
}

ScriptGraphNode::PinConnection ConfigNodeSerializer<ScriptGraphNode::PinConnection>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptGraphNode::PinConnection(node, context);
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
