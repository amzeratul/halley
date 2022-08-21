#include "scripting/script_graph.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "halley/utils/hash.h"
#include "nodes/script_messaging.h"
#include "scripting/script_node_type.h"
using namespace Halley;

ScriptGraphNode::PinConnection::PinConnection(const ConfigNode& node)
{
	if (node.hasKey("dstNode")) {
		dstNode = static_cast<GraphNodeId>(node["dstNode"].asInt());
	}
	if (node.hasKey("entityIdx")) {
		entityIdx = node["entityIdx"].asInt();
	}
	dstPin = static_cast<GraphPinId>(node["dstPin"].asInt(0));
}

ScriptGraphNode::PinConnection::PinConnection(GraphNodeId dstNode, GraphPinId dstPin)
	: dstNode(dstNode)
	, dstPin(dstPin)
{
}

ScriptGraphNode::PinConnection::PinConnection(OptionalLite<GraphPinId> entityIdx)
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

bool ScriptGraphNode::PinConnection::hasConnection() const
{
	return dstNode || entityIdx;
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

bool ScriptGraphNode::Pin::hasConnection() const
{
	return std::any_of(connections.begin(), connections.end(), [] (const PinConnection& p) { return p.hasConnection(); });
}

ScriptGraphNode::ScriptGraphNode()
{}

ScriptGraphNode::ScriptGraphNode(String type, Vector2f position)
	: BaseGraphNode(type, position)
{
}

ScriptGraphNode::ScriptGraphNode(const ConfigNode& node)
	: BaseGraphNode(node)
{
}

void ScriptGraphNode::serialize(Serializer& s) const
{
	BaseGraphNode::serialize(s);
	s << parentNode;
}

void ScriptGraphNode::deserialize(Deserializer& s)
{
	BaseGraphNode::deserialize(s);
	s >> parentNode;
}

void ScriptGraphNode::feedToHash(Hash::Hasher& hasher)
{
	BaseGraphNode::feedToHash(hasher);
}

void ScriptGraphNode::offsetNodes(GraphNodeId offset)
{
	BaseGraphNode::offsetNodes(offset);
	if (parentNode) {
		*parentNode += offset;
	}
}

void ScriptGraphNode::assignType(const ScriptNodeTypeCollection& nodeTypeCollection) const
{
	nodeType = nodeTypeCollection.tryGetNodeType(type);
	Ensures(nodeType != nullptr);
}

const IScriptNodeType& ScriptGraphNode::getNodeType() const
{
	Expects(nodeType != nullptr);
	return *nodeType;
}

GraphNodePinType ScriptGraphNode::getPinType(GraphPinId idx) const
{
	const auto& config = getNodeType().getPinConfiguration(*this);
	if (idx >= config.size()) {
		return GraphNodePinType();
	}
	return config[idx];
}

ScriptGraphNodeRoots::Entry::Entry(Range<GraphNodeId> range, GraphNodeId root)
	: range(range)
	, root(root)
{
}

ScriptGraphNodeRoots::Entry::Entry(const ConfigNode& node)
{
	auto r = node["range"].asVector2i();
	range.start = r.x;
	range.end = r.y;
	root = node["root"].asInt();
}

ConfigNode ScriptGraphNodeRoots::Entry::toConfigNode() const
{
	ConfigNode::MapType result;
	result["range"] = Vector2i(range.start, range.end);
	result["root"] = int(root);
	return result;
}

ScriptGraphNodeRoots::ScriptGraphNodeRoots(const ConfigNode& node)
{
	mapping = node.asVector<Entry>();
}

ConfigNode ScriptGraphNodeRoots::toConfigNode() const
{
	return ConfigNode(mapping);
}

void ScriptGraphNodeRoots::addRoot(GraphNodeId id, GraphNodeId root)
{
	if (!mapping.empty() && mapping.back().root == root && mapping.back().range.end == id) {
		mapping.back().range.end++;
	} else {
		mapping.emplace_back(Range<GraphNodeId>(id, id + 1), root);
	}
}

GraphNodeId ScriptGraphNodeRoots::getRoot(GraphNodeId id) const
{
	for (auto& e: mapping) {
		if (e.range.contains(id)) {
			return e.root;
		}
	}
	return id;
}

void ScriptGraphNodeRoots::clear()
{
	mapping.clear();
}

ScriptGraph::ScriptGraph()
{
	finishGraph();
}

ScriptGraph::ScriptGraph(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		nodes = node["nodes"].asVector<ScriptGraphNode>({});
	}
	finishGraph();
}

ScriptGraph::ScriptGraph(const ConfigNode& node, const EntitySerializationContext& context)
{
	load(node, context);
}

void ScriptGraph::load(const ConfigNode& node, const EntitySerializationContext& context)
{
	nodes = node["nodes"].asVector<ScriptGraphNode>({});
	entityIds = ConfigNodeSerializer<Vector<EntityId>>().deserialize(context, node["entityIds"]);
	lastAssignTypeHash = 0;
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

String ScriptGraph::toYAML() const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = { "type", "settings", "position", "pins" };
	options.compactMaps = true;
	return YAMLConvert::generateYAML(toConfigNode(), options);
}

std::shared_ptr<ScriptGraph> ScriptGraph::loadResource(ResourceLoader& loader)
{
	auto script = std::make_shared<ScriptGraph>();
	Deserializer::fromBytes(*script, loader.getStatic()->getSpan(), SerializerOptions(SerializerOptions::maxVersion));
	script->generateRoots();
	return script;
}

void ScriptGraph::reload(Resource&& resource)
{
	*this = dynamic_cast<ScriptGraph&&>(resource);
}

void ScriptGraph::makeDefault()
{
	nodes.clear();
	entityIds.clear();
	finishGraph();
}

void ScriptGraph::serialize(Serializer& s) const
{
	s << nodes;
	s << callerToCallee;
	s << returnToCaller;
	s << subGraphs;
}

void ScriptGraph::deserialize(Deserializer& s)
{
	s >> nodes;
	s >> callerToCallee;
	s >> returnToCaller;
	s >> subGraphs;
	finishGraph();
}

void ScriptGraph::makeBaseGraph()
{
	nodes.emplace_back("start", Vector2f(0, -30));
}

OptionalLite<GraphNodeId> ScriptGraph::getStartNode() const
{
	const auto iter = std::find_if(nodes.begin(), nodes.end(), [&] (const ScriptGraphNode& node) { return node.getType() == "start"; });
	if (iter != nodes.end()) {
		return static_cast<GraphNodeId>(iter - nodes.begin());
	}
	return {};
}

OptionalLite<GraphNodeId> ScriptGraph::getCallee(GraphNodeId node) const
{
	const auto iter = std_ex::find_if(callerToCallee, [&] (auto& e) { return e.first == node; });
	if (iter != callerToCallee.end()) {
		return iter->second;
	}
	return std::nullopt;
}

OptionalLite<GraphNodeId> ScriptGraph::getCaller(GraphNodeId node) const
{
	const auto iter = std_ex::find_if(callerToCallee, [&] (auto& e) { return e.second == node; });
	if (iter != callerToCallee.end()) {
		return iter->first;
	}
	return std::nullopt;
}

OptionalLite<GraphNodeId> ScriptGraph::getReturnTo(GraphNodeId node) const
{
	const auto iter = std_ex::find_if(returnToCaller, [&] (auto& e) { return e.first == node; });
	if (iter != returnToCaller.end()) {
		return iter->second;
	}
	return std::nullopt;
}

OptionalLite<GraphNodeId> ScriptGraph::getReturnFrom(GraphNodeId node) const
{
	const auto iter = std_ex::find_if(returnToCaller, [&] (auto& e) { return e.second == node; });
	if (iter != returnToCaller.end()) {
		return iter->first;
	}
	return std::nullopt;
}

uint64_t ScriptGraph::getHash() const
{
	return hash;
}

std::optional<GraphNodeId> ScriptGraph::getMessageInboxId(const String& messageId, bool requiresSpawningScript) const
{
	for (size_t i = 0; i < nodes.size(); ++i) {
		const auto& node = nodes[i];
		if (node.getType() == "receiveMessage") {
			ScriptReceiveMessage nodeType;
			const bool ok = nodeType.canReceiveMessage(node, messageId, requiresSpawningScript);
			if (ok) {
				return static_cast<GraphNodeId>(i);
			}
		}
	}
	return {};
}

Vector<String> ScriptGraph::getMessageNames() const
{
	Vector<String> result;
	for (size_t i = 0; i < nodes.size(); ++i) {
		const auto& node = nodes[i];
		if (node.getType() == "receiveMessage") {
			ScriptReceiveMessage nodeType;
			auto [msg, params] = nodeType.getMessageIdAndParams(node);
			result.push_back(std::move(msg));
		}
	}
	return result;
}

int ScriptGraph::getMessageNumParams(const String& messageId) const
{
	for (size_t i = 0; i < nodes.size(); ++i) {
		const auto& node = nodes[i];
		if (node.getType() == "receiveMessage") {
			ScriptReceiveMessage nodeType;
			const auto [msg, params] = nodeType.getMessageIdAndParams(node);
			if (msg == messageId) {
				return params;
			}
		}
	}
	return 0;
}

bool ScriptGraph::connectPin(GraphNodeId srcNodeIdx, GraphPinId srcPinN, EntityId target)
{
	auto& srcNode = nodes.at(srcNodeIdx);
	auto& srcPin = srcNode.getPin(srcPinN);

	for (const auto& conn: srcPin.connections) {
		if (getEntityId(conn.entityIdx) == target) {
			return false;
		}
	}

	disconnectPinIfSingleConnection(srcNodeIdx, srcPinN);

	if (target.isValid()) {
		srcPin.connections.emplace_back(ScriptGraphNode::PinConnection{ addEntityId(target) });
	}

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

EntityId ScriptGraph::getEntityId(OptionalLite<uint8_t> idx) const
{
	if (!idx) {
		return EntityId();
	}
	return entityIds.at(idx.value());
}

OptionalLite<uint8_t> ScriptGraph::getEntityIdx(EntityId id) const
{
	const auto iter = std_ex::find(entityIds, id);
	if (iter == entityIds.end()) {
		return {};
	} else {
		return static_cast<uint8_t>(iter - entityIds.begin());
	}
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

GraphNodeId ScriptGraph::getNodeRoot(GraphNodeId nodeId) const
{
	return roots.getRoot(nodeId);
}

GraphNodeId ScriptGraph::findNodeRoot(GraphNodeId nodeId) const
{
	const auto parent = nodes[nodeId].getParentNode();
	if (parent) {
		return getNodeRoot(*parent);
	}
	return nodeId;
}

const ScriptGraphNodeRoots& ScriptGraph::getRoots() const
{
	return roots;
}

void ScriptGraph::generateRoots()
{
	roots.clear();
	for (size_t curNodeId = 0; curNodeId < nodes.size(); ++curNodeId) {
		const auto parentId = findNodeRoot(static_cast<GraphNodeId>(curNodeId));
		if (curNodeId != parentId) {
			roots.addRoot(static_cast<GraphNodeId>(curNodeId), parentId);
		}
	}
}

bool ScriptGraph::isMultiConnection(GraphNodePinType pinType) const {
	return (pinType.type == static_cast<int>(ScriptNodeElementType::ReadDataPin) && pinType.direction == GraphNodePinDirection::Output)
		|| (pinType.type == static_cast<int>(ScriptNodeElementType::WriteDataPin) && pinType.direction == GraphNodePinDirection::Input)
		|| (pinType.type == static_cast<int>(ScriptNodeElementType::FlowPin))
		|| (pinType.type == static_cast<int>(ScriptNodeElementType::TargetPin) && pinType.direction == GraphNodePinDirection::Output);
}

void ScriptGraph::setRoots(ScriptGraphNodeRoots roots)
{
	this->roots = std::move(roots);
}

ScriptGraph::FunctionParameters ScriptGraph::getFunctionParameters() const
{
	FunctionParameters result;

	for (auto& node: nodes) {
		if (node.getType() == "start") {
			result.nDataInput = static_cast<uint8_t>(node.getSettings()["dataPins"].getSequenceSize());
			result.nTargetInput = static_cast<uint8_t>(node.getSettings()["targetPins"].getSequenceSize());
			auto dataPins = node.getSettings()["dataPins"].asVector<String>({});
			auto targetPins = node.getSettings()["targetPins"].asVector<String>({});
			result.inputNames = std::move(dataPins);
			std::move(targetPins.begin(), targetPins.end(), std::back_inserter(result.inputNames));
		} else if (node.getType() == "return") {
			result.nOutput = static_cast<uint8_t>(node.getSettings()["flowPins"].getSequenceSize(1));
			result.nDataOutput = static_cast<uint8_t>(node.getSettings()["dataPins"].getSequenceSize());
			result.nTargetOutput = static_cast<uint8_t>(node.getSettings()["targetPins"].getSequenceSize());
			auto flowPins = node.getSettings()["flowPins"].asVector<String>({});
			auto dataPins = node.getSettings()["dataPins"].asVector<String>({});
			auto targetPins = node.getSettings()["targetPins"].asVector<String>({});
			result.outputNames = std::move(flowPins);
			std::move(dataPins.begin(), dataPins.end(), std::back_inserter(result.outputNames));
			std::move(targetPins.begin(), targetPins.end(), std::back_inserter(result.outputNames));
			result.icon = node.getSettings()["icon"].asString("");
		}
	}

	return result;
}

void ScriptGraph::finishGraph()
{
	if (nodes.empty()) {
		makeBaseGraph();
	}

	Hash::Hasher hasher;
	GraphNodeId i = 0;
	hasher.feed(nodes.size());
	for (auto& node: nodes) {
		node.setId(i++);
		node.feedToHash(hasher);
	}
	hash = hasher.digest();
}

void ScriptGraph::appendGraph(GraphNodeId parent, const ScriptGraph& other)
{
	const auto offset = static_cast<GraphNodeId>(nodes.size());

	GraphNodeId startNode = offset;
	GraphNodeId returnNode = offset;

	for (size_t i = 0; i < other.getNodes().size(); ++i) {
		auto& node = nodes.emplace_back(other.getNodes()[i]);
		node.offsetNodes(offset);
		node.setParentNode(parent);
		if (node.getType() == "start") {
			startNode = offset + static_cast<GraphNodeId>(i);
		}
		if (node.getType() == "return") {
			returnNode = offset + static_cast<GraphNodeId>(i);
		}
	}

	callerToCallee.emplace_back(parent, startNode);
	returnToCaller.emplace_back(returnNode, parent);
	subGraphs.emplace_back(other.getAssetId(), Range<GraphNodeId>(offset, static_cast<GraphNodeId>(nodes.size())));

	// For nested calls
	for (const auto& e: other.callerToCallee) {
		callerToCallee.emplace_back(e.first + offset, e.second + offset);
	}
	for (const auto& e: other.returnToCaller) {
		returnToCaller.emplace_back(e.first + offset, e.second + offset);
	}
	for (const auto& e: other.subGraphs) {
		subGraphs.emplace_back(e.first, e.second + offset);
	}
}

Vector<int> ScriptGraph::getSubGraphIndicesForAssetId(const String& id) const
{
	Vector<int> result;
	if (id == getAssetId()) {
		result.push_back(-1);
	}
	for (int i = 0; i < int(subGraphs.size()); ++i) {
		if (subGraphs[i].first == id) {
			result.push_back(i);
		}
	}

	return result;
}

Range<GraphNodeId> ScriptGraph::getSubGraphRange(int subGraphIdx) const
{
	if (subGraphIdx == -1) {
		return {0, static_cast<GraphNodeId>(nodes.size())};
	} else {
		return subGraphs[subGraphIdx].second;
	}
}

ConfigNode ConfigNodeSerializer<ScriptGraph>::serialize(ScriptGraph script, const EntitySerializationContext& context)
{
	return script.toConfigNode(context);
}

ScriptGraph ConfigNodeSerializer<ScriptGraph>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return ScriptGraph(node, context);
}

void ConfigNodeSerializer<ScriptGraph>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptGraph& target)
{
	target.load(node, context);
}
