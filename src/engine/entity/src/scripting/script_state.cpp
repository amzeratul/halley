#include "scripting/script_state.h"

#include "halley/bytes/byte_serializer.h"
using namespace Halley;

ScriptStateThread::ScriptStateThread()
{
}

ScriptStateThread::ScriptStateThread(const ConfigNode& node, const ConfigNodeSerializationContext& context)
{
	timeSlice = node["timeSlice"].asFloat(0);
	if (node.hasKey("curNode")) {
		curNode = node["curNode"].asInt();
	}
	if (node.hasKey("curData")) {
		pendingData = node["curData"];
	}
}

ConfigNode ScriptStateThread::toConfigNode(const ConfigNodeSerializationContext& context) const
{
	ConfigNode::MapType node;
	if (timeSlice != 0) {
		node["timeSlice"] = timeSlice;
	}
	if (curNode) {
		node["curNode"] = static_cast<int>(curNode.value());
	}
	if (curData) {
		node["curData"] = curData->toConfigNode(context);
	}
	return node;
}

bool ScriptStateThread::hasPendingNodeData() const
{
	return pendingData.getType() != ConfigNodeType::Undefined;
}

ConfigNode ScriptStateThread::getPendingNodeData()
{
	return std::move(pendingData);
}

ScriptStateThread::ScriptStateThread(uint32_t startNode)
	: curNode(startNode)
	, nodeStarted(false)
{
}

ScriptStateThread::ScriptStateThread(const ScriptStateThread& other)
{
	*this = other;
}

ScriptStateThread& ScriptStateThread::operator=(const ScriptStateThread& other)
{
	Expects(!curData);
	curNode = other.curNode;
	nodeStarted = other.nodeStarted;
	timeSlice = other.timeSlice;
	return *this;
}

void ScriptStateThread::startNode(std::unique_ptr<IScriptStateData> data)
{
	Expects(!nodeStarted);
	nodeStarted = true;
	curData = std::move(data);
}

void ScriptStateThread::finishNode()
{
	Expects(nodeStarted);
	nodeStarted = false;
	curData.reset();
}

void ScriptStateThread::advanceToNode(OptionalLite<uint32_t> node)
{
	curNode = node;
}

ScriptState::ScriptState()
{}

ScriptState::ScriptState(const ConfigNode& node, const ConfigNodeSerializationContext& context)
{
	started = node["started"].asBool(false);
	threads = ConfigNodeSerializer<decltype(threads)>().deserialize(context, node["threads"]);
	graphHash = Deserializer::fromBytes<decltype(graphHash)>(node["graphHash"].asBytes());
	variables = ConfigNodeSerializer<decltype(variables)>().deserialize(context, node["variables"]);
}

ConfigNode ScriptState::toConfigNode(const ConfigNodeSerializationContext& context) const
{
	ConfigNode::MapType node;
	if (started) {
		node["started"] = started;
	}
	node["threads"] = ConfigNodeSerializer<decltype(threads)>().serialize(threads, context);
	node["graphHash"] = Serializer::toBytes(graphHash);
	node["variables"] = ConfigNodeSerializer<decltype(variables)>().serialize(variables, context);
	return node;
}

bool ScriptState::hasThreadAt(uint32_t node) const
{
	for (const auto& thread: threads) {
		if (thread.getCurNode() == node) {
			return true;
		}
	}
	return false;
}

void ScriptState::start(OptionalLite<uint32_t> startNode, uint64_t hash)
{
	threads.clear();
	nodeCounters.clear();
	if (startNode) {
		threads.emplace_back(startNode.value());
	}
	graphHash = hash;
	started = true;
}

void ScriptState::reset()
{
	threads.clear();
	nodeCounters.clear();
	started = false;
	graphHash = 0;
	for (auto& n: nodeIntrospection) {
		n.state = NodeIntrospectionState::Unvisited;
		n.time = 0;
	}
}

void ScriptState::setIntrospection(bool enabled)
{
	introspection = enabled;
	if (!introspection) {
		nodeIntrospection.clear();
	}
}

void ScriptState::updateIntrospection(Time t)
{
	const auto time = static_cast<float>(t);
	for (auto& n: nodeIntrospection) {
		n.time += time;
		n.activationTime = std::max(0.0f, n.activationTime - time);
	}
}

ScriptState::NodeIntrospection ScriptState::getNodeIntrospection(uint32_t nodeId) const
{
	return nodeId < nodeIntrospection.size() ? nodeIntrospection[nodeId] : NodeIntrospection();
}

size_t& ScriptState::getNodeCounter(uint32_t node)
{
	return nodeCounters[node];
}

ConfigNode ScriptState::getVariable(const String& name) const
{
	const auto iter = variables.find(name);
	if (iter != variables.end()) {
		return ConfigNode(iter->second);
	}
	return ConfigNode(0);
}

void ScriptState::setVariable(const String& name, ConfigNode value)
{
	variables[name] = std::move(value);
}

void ScriptState::onNodeStartedIntrospection(uint32_t nodeId)
{
	if (nodeId >= nodeIntrospection.size()) {
		nodeIntrospection.resize(nodeId + 1);
	}
	auto& node = nodeIntrospection[nodeId];
	node.state = NodeIntrospectionState::Active;
	node.time = 0;
	node.activationTime = 1.0f;
}

void ScriptState::onNodeEndedIntrospection(uint32_t nodeId)
{
	if (nodeId >= nodeIntrospection.size()) {
		nodeIntrospection.resize(nodeId + 1);
	}
	auto& node = nodeIntrospection.at(nodeId);
	node.state = NodeIntrospectionState::Visited;
	node.time = 0;
}

ConfigNode ConfigNodeSerializer<ScriptState>::serialize(const ScriptState& state, const ConfigNodeSerializationContext& context)
{
	return state.toConfigNode(context);
}

ScriptState ConfigNodeSerializer<ScriptState>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptState(node, context);
}

ConfigNode ConfigNodeSerializer<ScriptStateThread>::serialize(const ScriptStateThread& thread, const ConfigNodeSerializationContext& context)
{
	return thread.toConfigNode(context);
}

ScriptStateThread ConfigNodeSerializer<ScriptStateThread>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptStateThread(node, context);
}
