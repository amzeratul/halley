#include "scripting/script_graph.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/utils/algorithm.h"
#include "halley/utils/hash.h"
#include "scripting/script_node_type.h"
using namespace Halley;

ScriptGraphNode::PinConnection::PinConnection(const ConfigNode& node)
{
	if (node.hasKey("dstNode")) {
		dstNode = static_cast<uint32_t>(node["dstNode"].asInt());
	}
	if (node.hasKey("entityIdx")) {
		entityIdx = node["entityIdx"].asInt();
	}
	dstPin = static_cast<uint8_t>(node["dstPin"].asInt(0));
}

ScriptGraphNode::PinConnection::PinConnection(uint32_t dstNode, uint8_t dstPin)
	: dstNode(dstNode)
	, dstPin(dstPin)
{
}

ScriptGraphNode::PinConnection::PinConnection(OptionalLite<uint8_t> entityIdx)
	: entityIdx(entityIdx)
{
}

ConfigNode ScriptGraphNode::PinConnection::toConfigNode() const
{
	ConfigNode::MapType result;
	if (dstNode) {
		result["dstNode"] = ConfigNode(static_cast<int>(dstNode.value()));
	}
	if (dstPin != 0) {
		result["dstPin"] = static_cast<int>(dstPin);
	}
	if (entityIdx) {
		result["entityIdx"] = entityIdx.value();
	}
	return result;
}

void ScriptGraphNode::PinConnection::serialize(Serializer& s) const
{
	s << dstNode;
	s << dstPin;
	s << entityIdx;
}

void ScriptGraphNode::PinConnection::deserialize(Deserializer& s)
{
	s >> dstNode;
	s >> dstPin;
	s >> entityIdx;
}

ScriptGraphNode::Pin::Pin(const ConfigNode& node)
{
	connections = node.asVector<PinConnection>();
}

ConfigNode ScriptGraphNode::Pin::toConfigNode() const
{
	ConfigNode result;
	result = connections;
	return result;
}

void ScriptGraphNode::Pin::serialize(Serializer& s) const
{
	s << connections;
}

void ScriptGraphNode::Pin::deserialize(Deserializer& s)
{
	s >> connections;
}

ScriptGraphNode::ScriptGraphNode()
{}

ScriptGraphNode::ScriptGraphNode(String type, Vector2f position)
	: position(position)
	, type(std::move(type))
	, settings(ConfigNode::MapType())
{
}

ScriptGraphNode::ScriptGraphNode(const ConfigNode& node)
{
	position = node["position"].asVector2f();
	type = node["type"].asString();
	settings = ConfigNode(node["settings"]);
	pins = node["pins"].asVector<Pin>();
}

ConfigNode ScriptGraphNode::toConfigNode() const
{
	ConfigNode::MapType result;
	result["position"] = position;
	result["type"] = type;
	result["settings"] = ConfigNode(settings);
	result["pins"] = pins;
	return result;
}

void ScriptGraphNode::serialize(Serializer& s) const
{
	s << position;
	s << type;
	s << settings;
	s << pins;
}

void ScriptGraphNode::deserialize(Deserializer& s)
{
	s >> position;
	s >> type;
	s >> settings;
	s >> pins;
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
	const auto& config = getNodeType().getPinConfiguration(*this);
	if (idx >= config.size()) {
		return ScriptNodePinType();
	}
	return config[idx];
}

ScriptGraph::ScriptGraph()
{
	finishGraph();
}

ScriptGraph::ScriptGraph(const ConfigNode& node)
{
	nodes = node["nodes"].asVector<ScriptGraphNode>();
	finishGraph();
}

ScriptGraph::ScriptGraph(const ConfigNode& node, const EntitySerializationContext& context)
{
	nodes = node["nodes"].asVector<ScriptGraphNode>();
	entityIds = ConfigNodeSerializer<Vector<EntityId>>().deserialize(context, node["entityIds"]);
	finishGraph();
}

ConfigNode ScriptGraph::toConfigNode() const
{
	Expects(entityIds.empty());

	ConfigNode::MapType result;
	result["nodes"] = nodes;
	return result;
}

ConfigNode ScriptGraph::toConfigNode(const EntitySerializationContext& context) const
{
	ConfigNode::MapType result;
	result["nodes"] = nodes;
	result["entityIds"] = ConfigNodeSerializer<Vector<EntityId>>().serialize(entityIds, context);
	return result;
}

std::shared_ptr<ScriptGraph> ScriptGraph::loadResource(ResourceLoader& loader)
{
	auto script = std::make_shared<ScriptGraph>();
	Deserializer::fromBytes(*script, loader.getStatic()->getSpan(), SerializerOptions(SerializerOptions::maxVersion));
	return script;
}

void ScriptGraph::reload(Resource&& resource)
{
	*this = dynamic_cast<ScriptGraph&&>(resource);
}

void ScriptGraph::makeDefault()
{
}

void ScriptGraph::serialize(Serializer& s) const
{
	s << nodes;
}

void ScriptGraph::deserialize(Deserializer& s)
{
	s >> nodes;
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
		if (getEntityId(conn.entityIdx) == target) {
			return false;
		}
	}

	disconnectPinIfSingleConnection(srcNodeIdx, srcPinN);

	srcPin.connections.emplace_back(ScriptGraphNode::PinConnection{ addEntityId(target) });

	return true;
}

bool ScriptGraph::disconnectPin(uint32_t nodeIdx, uint8_t pinN)
{
	auto& node = nodes.at(nodeIdx);
	auto& pin = node.getPin(pinN);
	if (pin.connections.empty()) {
		return false;
	}

	for (auto& conn: pin.connections) {
		if (conn.dstNode) {
			auto& otherNode = nodes.at(conn.dstNode.value());
			auto& ocs = otherNode.getPin(conn.dstPin).connections;
			std_ex::erase_if(ocs, [&] (const auto& oc) { return oc.dstNode == nodeIdx && oc.dstPin == pinN; });
		}
	}

	pin.connections.clear();

	return true;
}

bool ScriptGraph::disconnectPinIfSingleConnection(uint32_t nodeIdx, uint8_t pinN)
{
	auto& node = nodes.at(nodeIdx);
	if (node.getPinType(pinN).isMultiConnection()) {
		return false;
	}

	return disconnectPin(nodeIdx, pinN);
}

void ScriptGraph::validateNodePins(uint32_t nodeIdx)
{
	auto& node = nodes.at(nodeIdx);

	const size_t nPinsCur = node.getPins().size();
	const size_t nPinsTarget = node.getNodeType().getPinConfiguration(node).size();
	if (nPinsCur > nPinsTarget) {
		for (size_t i = nPinsTarget; i < nPinsCur; ++i) {
			disconnectPin(nodeIdx, static_cast<uint8_t>(i));
		}
		node.getPins().resize(nPinsTarget);
	}
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

EntityId ScriptGraph::getEntityId(OptionalLite<uint8_t> idx) const
{
	if (!idx) {
		return EntityId();
	}
	return entityIds.at(idx.value());
}

uint8_t ScriptGraph::addEntityId(EntityId id)
{
	// Look for existing
	size_t empty = std::numeric_limits<size_t>::max();
	for (size_t i = 0; i < entityIds.size(); ++i) {
		if (entityIds[i] == id) {
			return static_cast<uint8_t>(i);
		} else if (!entityIds[i].isValid() && empty == std::numeric_limits<size_t>::max()) {
			empty = i;
		}
	}

	// Fill empty slot
	if (empty != std::numeric_limits<size_t>::max()) {
		entityIds[empty] = id;
		return static_cast<uint8_t>(empty);
	}

	// Add new slot
	const auto idx = entityIds.size();
	if (idx >= 255) {
		throw Exception("Too many entityIds in ScriptGraph", HalleyExceptions::Entity);
	}
	entityIds.push_back(id);
	return static_cast<uint8_t>(idx);
}

void ScriptGraph::removeEntityId(EntityId id)
{
	for (size_t i = 0; i < entityIds.size(); ++i) {
		if (entityIds[i] == id) {
			entityIds[i] = EntityId();
		}
	}
}

void ScriptGraph::finishGraph()
{
	if (nodes.empty()) {
		makeBaseGraph();
	}

	Hash::Hasher hasher;
	uint32_t i = 0;
	for (auto& node: nodes) {
		node.setId(i++);
		node.feedToHash(hasher);
	}
	hash = hasher.digest();
}

ConfigNode ConfigNodeSerializer<ScriptGraph>::serialize(ScriptGraph script, const EntitySerializationContext& context)
{
	return script.toConfigNode(context);
}

ScriptGraph ConfigNodeSerializer<ScriptGraph>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return ScriptGraph(node, context);
}
