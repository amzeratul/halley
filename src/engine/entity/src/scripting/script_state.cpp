#include "scripting/script_state.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"
#include "scripting/script_graph.h"
#include "scripting/script_node_type.h"
using namespace Halley;

ScriptStateThread::StackFrame::StackFrame(const ConfigNode& n)
{
	auto v = n.asVector2i();
	node = v.x;
	pin = v.y;
}

ScriptStateThread::StackFrame::StackFrame(ScriptNodeId node, ScriptPinId pin)
	: node(node)
	, pin(pin)
{
}

ConfigNode ScriptStateThread::StackFrame::toConfigNode() const
{
	return ConfigNode(Vector2i(node, pin));
}

ScriptStateThread::ScriptStateThread()
{
	stack.reserve(16);
}

ScriptStateThread::ScriptStateThread(ScriptNodeId startNode)
	: curNode(startNode)
{
}

ScriptStateThread::ScriptStateThread(const ScriptStateThread& other)
{
	*this = other;
}

ScriptStateThread::ScriptStateThread(const ConfigNode& node, const EntitySerializationContext& context)
{
	stack = node["stack"].asVector<StackFrame>();
	timeSlice = node["timeSlice"].asFloat(0);
	if (node.hasKey("curNode")) {
		curNode = node["curNode"].asInt();
	}
}

ScriptStateThread::~ScriptStateThread()
{
	if (!stack.empty()) {
		Logger::logError("ScriptStateThread terminated with a non-empty stack");
	}
}

ScriptStateThread& ScriptStateThread::operator=(const ScriptStateThread& other)
{
	curNode = other.curNode;
	timeSlice = other.timeSlice;
	return *this;
}

ConfigNode ScriptStateThread::toConfigNode(const EntitySerializationContext& context) const
{
	ConfigNode::MapType node;
	node["stack"] = stack;
	if (timeSlice != 0) {
		node["timeSlice"] = timeSlice;
	}
	if (curNode) {
		node["curNode"] = static_cast<int>(curNode.value());
	}
	return node;
}

const Vector<ScriptStateThread::StackFrame>& ScriptStateThread::getStack() const
{
	return stack;
}

Vector<ScriptStateThread::StackFrame>& ScriptStateThread::getStack()
{
	return stack;
}

bool ScriptStateThread::stackGoesThrough(ScriptNodeId node, std::optional<ScriptPinId> pin) const
{
	return !stack.empty() && std::any_of(stack.begin(), stack.end(), [&] (const StackFrame& frame)
	{
		return frame.node == node && (!pin || frame.pin == pin);
	});
}

void ScriptStateThread::advanceToNode(OptionalLite<ScriptNodeId> node, ScriptPinId outputPin)
{
	if (curNode) {
		stack.push_back(StackFrame(*curNode, outputPin));
	}
	curNode = node;
}

ScriptStateThread ScriptStateThread::fork(OptionalLite<ScriptNodeId> node, ScriptPinId outputPin) const
{
	ScriptStateThread newThread = *this;
	newThread.advanceToNode(node, outputPin);
	return newThread;
}

ScriptState::NodeState::NodeState()
	: data(nullptr)
{
}

ScriptState::NodeState::NodeState(const ConfigNode& node, const EntitySerializationContext& context)
	: data(nullptr)
{
	threadCount = static_cast<uint8_t>(node["threadCount"].asInt(0));
	pendingData = new ConfigNode(node["pendingData"]);
	hasPendingData = true;
}

ScriptState::NodeState::NodeState(const NodeState& other)
	: data(nullptr)
{
	*this = other;
}

ScriptState::NodeState::NodeState(NodeState&& other)
	: data(nullptr)
{
	*this = std::move(other);
}

ScriptState::NodeState& ScriptState::NodeState::operator=(const NodeState& other)
{
	releaseData();

	if (other.data != nullptr) {
		throw Exception("Invalid copy operation", HalleyExceptions::Entity);
	}

	threadCount = other.threadCount;

	return *this;
}

ScriptState::NodeState& ScriptState::NodeState::operator=(NodeState&& other)
{
	releaseData();

	threadCount = other.threadCount;

	data = other.data;
	hasPendingData = other.hasPendingData;

	other.data = nullptr;
	other.hasPendingData = false;

	return *this;
}

ConfigNode ScriptState::NodeState::toConfigNode(const EntitySerializationContext& context) const
{
	ConfigNode::MapType result;
	result["threadCount"] = threadCount;
	if (hasPendingData) {
		if (pendingData) {
			result["pendingData"] = ConfigNode(*pendingData);
		}
	} else if (data) {
		result["pendingData"] = data->toConfigNode(context);
	}
	return result;
}

void ScriptState::NodeState::releaseData()
{
	if (hasPendingData) {
		delete pendingData;
	} else {
		delete data;
	}
	data = nullptr;
}

ScriptState::NodeState::~NodeState()
{
	releaseData();
}

ScriptState::ScriptState()
{
	
}

ScriptState::ScriptState(const ConfigNode& node, const EntitySerializationContext& context)
{
	started = node["started"].asBool(false);
	threads = ConfigNodeSerializer<decltype(threads)>().deserialize(context, node["threads"]);
	nodeState = ConfigNodeSerializer<decltype(nodeState)>().deserialize(context, node["nodeState"]);
	graphHash = Deserializer::fromBytes<decltype(graphHash)>(node["graphHash"].asBytes());
	variables = ConfigNodeSerializer<decltype(variables)>().deserialize(context, node["variables"]);
	persistAfterDone = node["persistAfterDone"].asBool(false);

	const auto scriptGraphName = node["script"].asString();
	if (!scriptGraphName.isEmpty()) {
		scriptGraph = context.resources->get<ScriptGraph>(scriptGraphName);
	}
}

ScriptState::ScriptState(const ScriptGraph* script, bool persistAfterDone)
	: scriptGraphRef(script)
	, persistAfterDone(persistAfterDone)
{
}

ScriptState::ScriptState(std::shared_ptr<const ScriptGraph> script)
	: scriptGraph(script)
{
}

const ScriptGraph* ScriptState::getScriptGraphPtr() const
{
	return scriptGraph ? scriptGraph.get() : scriptGraphRef;
}

void ScriptState::setScriptGraphPtr(const ScriptGraph* script)
{
	scriptGraph.reset();
	scriptGraphRef = script;
}

bool ScriptState::isDone() const
{
	return started && threads.empty();
}

bool ScriptState::isDead() const
{
	return isDone() && !persistAfterDone;
}

ConfigNode ScriptState::toConfigNode(const EntitySerializationContext& context) const
{
	ConfigNode::MapType node;
	if (started) {
		node["started"] = started;
	}
	node["threads"] = ConfigNodeSerializer<decltype(threads)>().serialize(threads, context);
	node["nodeState"] = ConfigNodeSerializer<decltype(nodeState)>().serialize(nodeState, context);
	node["graphHash"] = Serializer::toBytes(graphHash);
	node["variables"] = ConfigNodeSerializer<decltype(variables)>().serialize(variables, context);
	node["script"] = scriptGraph ? scriptGraph->getAssetId() : "";
	node["persistAfterDone"] = persistAfterDone;
	return node;
}

bool ScriptState::hasThreadAt(ScriptNodeId node) const
{
	for (const auto& thread: threads) {
		if (thread.getCurNode() == node) {
			return true;
		}
	}
	return false;
}

void ScriptState::start(OptionalLite<ScriptNodeId> startNode, uint64_t hash)
{
	threads.clear();
	nodeCounters.clear();
	if (startNode) {
		threads.emplace_back(startNode.value());
	}
	graphHash = hash;
	started = true;
	nodeState.resize(getScriptGraphPtr()->getNodes().size());
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

ScriptState::NodeIntrospection ScriptState::getNodeIntrospection(ScriptNodeId nodeId) const
{
	return nodeId < nodeIntrospection.size() ? nodeIntrospection[nodeId] : NodeIntrospection();
}

size_t& ScriptState::getNodeCounter(ScriptNodeId node)
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

bool ScriptState::operator==(const ScriptState& other) const
{
	return false;
}

bool ScriptState::operator!=(const ScriptState& other) const
{
	return true;
}

ScriptState::NodeState& ScriptState::getNodeState(ScriptNodeId nodeId)
{
	return nodeState.at(nodeId);
}

void ScriptState::startNode(const ScriptGraphNode& node, NodeState& state)
{
	auto& nodeType = node.getNodeType();
	auto data = nodeType.makeData();
	if (data) {
		nodeType.initData(*data, node, state.hasPendingData ? std::move(*state.pendingData) : ConfigNode());
		state.data = data.release();
		state.hasPendingData = false;
	}
}

void ScriptState::finishNode(const ScriptGraphNode& node, NodeState& state)
{
	if (!node.getNodeType().canKeepData()) {
		state.releaseData();
	}
}

void ScriptState::onNodeStartedIntrospection(ScriptNodeId nodeId)
{
	if (nodeId >= nodeIntrospection.size()) {
		nodeIntrospection.resize(nodeId + 1);
	}
	auto& node = nodeIntrospection[nodeId];
	node.state = NodeIntrospectionState::Active;
	node.time = 0;
	node.activationTime = 1.0f;
}

void ScriptState::onNodeEndedIntrospection(ScriptNodeId nodeId)
{
	if (nodeId >= nodeIntrospection.size()) {
		nodeIntrospection.resize(nodeId + 1);
	}
	auto& node = nodeIntrospection.at(nodeId);
	node.state = NodeIntrospectionState::Visited;
	node.time = 0;
}

ConfigNode ConfigNodeSerializer<ScriptState>::serialize(const ScriptState& state, const EntitySerializationContext& context)
{
	return state.toConfigNode(context);
}

ScriptState ConfigNodeSerializer<ScriptState>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return ScriptState(node, context);
}

ConfigNode ConfigNodeSerializer<ScriptStateThread>::serialize(const ScriptStateThread& thread, const EntitySerializationContext& context)
{
	return thread.toConfigNode(context);
}

ScriptStateThread ConfigNodeSerializer<ScriptStateThread>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return ScriptStateThread(node, context);
}

ConfigNode ConfigNodeSerializer<ScriptState::NodeState>::serialize(const ScriptState::NodeState& state, const EntitySerializationContext& context)
{
	return state.toConfigNode(context);
}

ScriptState::NodeState ConfigNodeSerializer<ScriptState::NodeState>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return ScriptState::NodeState(node, context);
}
