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
		dstNode = static_cast<ScriptNodeId>(node["dstNode"].asInt());
	}
	if (node.hasKey("entityIdx")) {
		entityIdx = node["entityIdx"].asInt();
	}
	dstPin = static_cast<ScriptPinId>(node["dstPin"].asInt(0));
}

ScriptGraphNode::PinConnection::PinConnection(ScriptNodeId dstNode, ScriptPinId dstPin)
	: dstNode(dstNode)
	, dstPin(dstPin)
{
}

ScriptGraphNode::PinConnection::PinConnection(OptionalLite<ScriptPinId> entityIdx)
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
	: position(position)
	, type(std::move(type))
	, settings(ConfigNode::MapType())
{
}

ScriptGraphNode::ScriptGraphNode(const ConfigNode& node)
{
	position = node["position"].asVector2f();
	type = node["type"].asString();
	if (node.hasKey("settings")) {
		settings = ConfigNode(node["settings"]);
	} else {
		settings = ConfigNode::MapType();
	}
	pins = node["pins"].asVector<Pin>();
}

ConfigNode ScriptGraphNode::toConfigNode() const
{
	ConfigNode::MapType result;
	result["position"] = position;
	result["type"] = type;
	if (settings.getType() == ConfigNodeType::Map && !settings.asMap().empty()) {
		result["settings"] = ConfigNode(settings);
	}
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
	hasher.feed(type);
	// TODO: settings, pins
}

void ScriptGraphNode::onNodeRemoved(ScriptNodeId nodeId)
{
	for (auto& pin: pins) {
		for (auto& o: pin.connections) {
			if (o.dstNode) {
				if (o.dstNode.value() == nodeId) {
					o.dstNode = OptionalLite<ScriptNodeId>();
					o.dstPin = 0;
				} else if (o.dstNode.value() >= nodeId) {
					--o.dstNode.value();
				}
			}
		}
		std_ex::erase_if(pin.connections, [] (const PinConnection& c) { return !c.hasConnection(); });
	}
}

void ScriptGraphNode::remapNodes(const HashMap<ScriptNodeId, ScriptNodeId>& remap)
{
	for (auto& pin: pins) {
		for (auto& o: pin.connections) {
			if (o.dstNode) {
				const auto iter = remap.find(o.dstNode.value());
				if (iter != remap.end()) {
					o.dstNode = iter->second;
				} else {
					o.dstNode.reset();
					o.dstPin = 0;
				}
			} else if (o.entityIdx) {
				o.entityIdx.reset();
			}
		}
		std_ex::erase_if(pin.connections, [] (const PinConnection& c) { return !c.hasConnection(); });
	}
}

void ScriptGraphNode::offsetNodes(ScriptNodeId offset)
{
	for (auto& pin: pins) {
		for (auto& o: pin.connections) {
			if (o.dstNode) {
				*o.dstNode += offset;
			}
		}
	}
	id += offset;
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

ScriptNodePinType ScriptGraphNode::getPinType(ScriptPinId idx) const
{
	const auto& config = getNodeType().getPinConfiguration(*this);
	if (idx >= config.size()) {
		return ScriptNodePinType();
	}
	return config[idx];
}

ScriptGraphNodeRoots::Entry::Entry(Range<ScriptNodeId> range, ScriptNodeId root)
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

void ScriptGraphNodeRoots::addRoot(ScriptNodeId id, ScriptNodeId root)
{
	if (!mapping.empty() && mapping.back().root == root && mapping.back().range.end == id) {
		mapping.back().range.end++;
	} else {
		mapping.emplace_back(Range<ScriptNodeId>(id, id + 1), root);
	}
}

ScriptNodeId ScriptGraphNodeRoots::getRoot(ScriptNodeId id) const
{
	for (auto& e: mapping) {
		if (e.range.contains(id)) {
			return e.root;
		}
	}
	return id;
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

void ScriptGraph::loadDependencies(const Resources& resources)
{
	bool modified = false;

	// NB: Call to appendGraph will append elements to nodes
	for (size_t i = 0; i < nodes.size(); ++i) {
		auto& node = nodes[i];
		if (node.getType() == "callExternal") {
			const auto function = node.getSettings()["function"].asString("");
			if (!function.isEmpty()) {
				if (resources.exists<ScriptGraph>(function)) {
					const ScriptNodeId nodeId = static_cast<ScriptNodeId>(i);
					const auto [startNode, returnNode] = appendGraph(nodeId, *resources.get<ScriptGraph>(function));
					callerToCallee.emplace_back(nodeId, startNode);
					returnToCaller.emplace_back(returnNode, nodeId);
					modified = true;
				} else {
					Logger::logError("Script \"" + function + "\" referenced by " + getAssetId() + " doesn't exist.");
				}
			}
		}
	}

	if (modified) {
		generateRoots();
		finishGraph();
	}
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
	script->loadDependencies(loader.getResources());
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
}

void ScriptGraph::deserialize(Deserializer& s)
{
	s >> nodes;
	finishGraph();
}

void ScriptGraph::makeBaseGraph()
{
	nodes.emplace_back("start", Vector2f(0, -30));
}

OptionalLite<ScriptNodeId> ScriptGraph::getStartNode() const
{
	const auto iter = std::find_if(nodes.begin(), nodes.end(), [&] (const ScriptGraphNode& node) { return node.getType() == "start"; });
	if (iter != nodes.end()) {
		return static_cast<ScriptNodeId>(iter - nodes.begin());
	}
	return {};
}

OptionalLite<ScriptNodeId> ScriptGraph::getCallee(ScriptNodeId node) const
{
	const auto iter = std_ex::find_if(callerToCallee, [&] (auto& e) { return e.first == node; });
	if (iter != callerToCallee.end()) {
		return iter->second;
	}
	return std::nullopt;
}

OptionalLite<ScriptNodeId> ScriptGraph::getCaller(ScriptNodeId node) const
{
	const auto iter = std_ex::find_if(callerToCallee, [&] (auto& e) { return e.second == node; });
	if (iter != callerToCallee.end()) {
		return iter->first;
	}
	return std::nullopt;
}

OptionalLite<ScriptNodeId> ScriptGraph::getReturnTo(ScriptNodeId node) const
{
	const auto iter = std_ex::find_if(returnToCaller, [&] (auto& e) { return e.first == node; });
	if (iter != returnToCaller.end()) {
		return iter->second;
	}
	return std::nullopt;
}

OptionalLite<ScriptNodeId> ScriptGraph::getReturnFrom(ScriptNodeId node) const
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

std::optional<ScriptNodeId> ScriptGraph::getMessageInboxId(const String& messageId, bool requiresSpawningScript) const
{
	for (size_t i = 0; i < nodes.size(); ++i) {
		const auto& node = nodes[i];
		if (node.getType() == "receiveMessage") {
			ScriptReceiveMessage nodeType;
			const bool ok = nodeType.canReceiveMessage(node, messageId, requiresSpawningScript);
			if (ok) {
				return static_cast<ScriptNodeId>(i);
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

bool ScriptGraph::connectPins(ScriptNodeId srcNodeIdx, ScriptPinId srcPinN, ScriptNodeId dstNodeIdx, ScriptPinId dstPinN)
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

bool ScriptGraph::connectPin(ScriptNodeId srcNodeIdx, ScriptPinId srcPinN, EntityId target)
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

bool ScriptGraph::disconnectPin(ScriptNodeId nodeIdx, ScriptPinId pinN)
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

bool ScriptGraph::disconnectPinIfSingleConnection(ScriptNodeId nodeIdx, ScriptPinId pinN)
{
	auto& node = nodes.at(nodeIdx);
	if (node.getPinType(pinN).isMultiConnection()) {
		return false;
	}

	return disconnectPin(nodeIdx, pinN);
}

void ScriptGraph::validateNodePins(ScriptNodeId nodeIdx)
{
	auto& node = nodes.at(nodeIdx);

	const size_t nPinsCur = node.getPins().size();
	const size_t nPinsTarget = node.getNodeType().getPinConfiguration(node).size();
	if (nPinsCur > nPinsTarget) {
		for (size_t i = nPinsTarget; i < nPinsCur; ++i) {
			disconnectPin(nodeIdx, static_cast<ScriptPinId>(i));
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

ScriptNodeId ScriptGraph::getNodeRoot(ScriptNodeId nodeId) const
{
	return roots.getRoot(nodeId);
}

ScriptNodeId ScriptGraph::findNodeRoot(ScriptNodeId nodeId) const
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
	for (size_t curNodeId = 0; curNodeId < nodes.size(); ++curNodeId) {
		const auto parentId = findNodeRoot(static_cast<ScriptNodeId>(curNodeId));
		if (curNodeId != parentId) {
			roots.addRoot(static_cast<ScriptNodeId>(curNodeId), parentId);
		}
	}
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
			result.nDataInput = node.getSettings()["dataPins"].asInt(0);
			result.nTargetInput = node.getSettings()["targetPins"].asInt(0);
		} else if (node.getType() == "return") {
			result.nOutput = node.getSettings()["flowPins"].asInt(1);
			result.nDataOutput = node.getSettings()["dataPins"].asInt(0);
			result.nTargetOutput = node.getSettings()["targetPins"].asInt(0);
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
	ScriptNodeId i = 0;
	hasher.feed(nodes.size());
	for (auto& node: nodes) {
		node.setId(i++);
		node.feedToHash(hasher);
	}
	hash = hasher.digest();
}

std::pair<ScriptNodeId, ScriptNodeId> ScriptGraph::appendGraph(ScriptNodeId parent, const ScriptGraph& other)
{
	const auto offset = static_cast<ScriptNodeId>(nodes.size());

	ScriptNodeId startNode = offset;
	ScriptNodeId returnNode = offset;

	for (size_t i = 0; i < other.getNodes().size(); ++i) {
		auto& node = nodes.emplace_back(other.getNodes()[i]);
		node.offsetNodes(offset);
		node.setParentNode(parent);
		if (node.getType() == "start") {
			startNode = offset + static_cast<ScriptNodeId>(i);
		}
		if (node.getType() == "return") {
			returnNode = offset + static_cast<ScriptNodeId>(i);
		}
	}
	return { startNode, returnNode };
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
