#include "halley/scripting/script_state.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "nodes/script_messaging.h"
#include "halley/scripting/script_graph.h"
#include "halley/scripting/script_node_type.h"
using namespace Halley;

ScriptStateThread::StackFrame::StackFrame(const ConfigNode& n)
{
	const auto result = n.asInt64();
	node = result & 0xFFFF;
	outputPin = (result >> 16) & 0xFF;
	inputPin = (result >> 24) & 0xFF;
}

ScriptStateThread::StackFrame::StackFrame(GraphNodeId node, GraphPinId outputPin, GraphPinId inputPin)
	: node(node)
	, outputPin(outputPin)
	, inputPin(inputPin)
{
}

ConfigNode ScriptStateThread::StackFrame::toConfigNode() const
{
	const int64_t result = static_cast<int64_t>(node) | (static_cast<int64_t>(outputPin) << 16) | (static_cast<int64_t>(inputPin) << 24);
	return ConfigNode(result);
}

String ScriptStateThread::StackFrame::toString() const
{
	return Halley::toString(node) + ":" + Halley::toString(static_cast<int>(outputPin)) + "->" + Halley::toString(static_cast<int>(inputPin));
}

bool ScriptStateThread::StackFrame::operator==(const StackFrame& other) const
{
	return node == other.node && outputPin == other.outputPin;
}

bool ScriptStateThread::StackFrame::operator!=(const StackFrame& other) const
{
	return node != other.node || outputPin != other.outputPin;
}

ScriptStateThread::ScriptStateThread()
	: merging(false)
	, watcher(false)
{
	stack.reserve(16);
	generateId();
}

ScriptStateThread::ScriptStateThread(GraphNodeId startNode, GraphPinId inputPin)
	: ScriptStateThread()
{
	curNode = startNode;
	curInputPin = inputPin;
}

ScriptStateThread::ScriptStateThread(const ConfigNode& node, const EntitySerializationContext& context)
	: ScriptStateThread()
{
	stack = node["stack"].asVector<StackFrame>();
	timeSlice = node["timeSlice"].asFloat(0);
	if (node.hasKey("curNode")) {
		curNode = node["curNode"].asInt();
	}
	if (node.hasKey("curInputPin")) {
		curInputPin = node["curInputPin"].asInt();
	}
	curNodeTime = node["curNodeTime"].asFloat(0);
	generateId();
}

bool ScriptStateThread::isRunning() const
{
	return curNode && !merging;
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
		if (curInputPin != 0) {
			node["curInputPin"] = static_cast<int>(curInputPin);
		}
	}

	if (context.matchType(EntitySerialization::makeMask(EntitySerialization::Type::DevCon))) {
		node["curNodeTime"] = curNodeTime;
	}

	return node;
}

void ScriptStateThread::merge(const ScriptStateThread& other)
{
	for (auto& f: other.stack) {
		if (!std_ex::contains(stack, f)) {
			stack.push_back(f);
		}
	}
}

const Vector<ScriptStateThread::StackFrame>& ScriptStateThread::getStack() const
{
	return stack;
}

Vector<ScriptStateThread::StackFrame>& ScriptStateThread::getStack()
{
	return stack;
}

bool ScriptStateThread::stackGoesThrough(GraphNodeId node, std::optional<GraphPinId> pin) const
{
	return std::any_of(stack.begin(), stack.end(), [&] (const StackFrame& frame)
	{
		return frame.node == node && (!pin || frame.outputPin == pin);
	});
}

uint32_t ScriptStateThread::getUniqueId() const
{
	return uniqueId;
}

void ScriptStateThread::offsetToNodeRange(Range<GraphNodeId> range)
{
	if (curNode) {
		*curNode -= range.start;
	}
	for (auto& s: stack) {
		s.node -= range.start;
	}
}

void ScriptStateThread::generateId()
{
	static uint32_t nextId = 0;
	uniqueId = nextId++;
}

void ScriptStateThread::advanceToNode(OptionalLite<GraphNodeId> node, GraphPinId outputPin, GraphPinId inputPin)
{
	if (curNode) {
		stack.push_back(StackFrame(*curNode, outputPin, curInputPin));
	}
	curNodeTime = 0;
	curNode = node;
	curInputPin = inputPin;
}

ScriptStateThread ScriptStateThread::fork(OptionalLite<GraphNodeId> node, GraphPinId outputPin, GraphPinId inputPin) const
{
	ScriptStateThread newThread = *this;
	newThread.generateId();
	newThread.advanceToNode(node, outputPin, inputPin);
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
	if (node.hasKey("pendingData")) {
		pendingData = new ConfigNode(node["pendingData"]);
		hasPendingData = true;
	}
	timeSinceStart = node["timeSinceStart"].asFloat(std::numeric_limits<float>::infinity());
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

	hasPendingData = other.hasPendingData;
	threadCount = other.threadCount;
	timeSinceStart = other.timeSinceStart;

	if (other.hasPendingData) {
		if (other.pendingData) {
			pendingData = new ConfigNode(*other.pendingData);
		}
	} else {
		if (other.data) {
			data = other.data->clone().release();
		}
	}

	return *this;
}

ScriptState::NodeState& ScriptState::NodeState::operator=(NodeState&& other)
{
	releaseData();

	threadCount = other.threadCount;
	timeSinceStart = other.timeSinceStart;

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

	if (context.matchType(EntitySerialization::makeMask(EntitySerialization::Type::DevCon))) {
		result["timeSinceStart"] = timeSinceStart;
	}

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

ScriptState::ScriptState(const ScriptGraph* script, bool persistAfterDone)
	: scriptGraphRef(script)
	, persistAfterDone(persistAfterDone)
{
}

ScriptState::ScriptState(std::shared_ptr<const ScriptGraph> script)
	: scriptGraph(script)
{
}

ScriptState::ScriptState(const ConfigNode& node, const EntitySerializationContext& context)
{
	load(node, context);
}

void ScriptState::load(const ConfigNode& node, const EntitySerializationContext& context)
{
	if (!context.matchType(EntitySerialization::makeMask(EntitySerialization::Type::Network))) {
		started = node["started"].asBool(false);
		threads = ConfigNodeSerializer<decltype(threads)>().deserialize(context, node["threads"]);
		nodeState = ConfigNodeSerializer<decltype(nodeState)>().deserialize(context, node["nodeState"]);
		graphHash = Deserializer::fromBytes<decltype(graphHash)>(node["graphHash"].asBytes());
		ConfigNodeSerializer<decltype(localVars)>().deserialize(context, node["localVars"], localVars);
		frameNumber = node["frameNumber"].asInt(0);
	}

	ConfigNodeSerializer<decltype(sharedVars)>().deserialize(context, node["sharedVars"], sharedVars);

	if (node.hasKey("persistAfterDone")) {
		persistAfterDone = node["persistAfterDone"].asBool();
	}

	if (node.hasKey("tags")) {
		tags = node["tags"].asVector<String>();
	}

	if (node.hasKey("script")) {
		const auto scriptGraphName = node["script"].asString();
		if (!scriptGraphName.isEmpty()) {
			scriptGraph = context.resources->get<ScriptGraph>(scriptGraphName);
		}
		needsStateLoading = true;
	}

	if (node.hasKey("startParams")) {
		ConfigNodeSerializer<decltype(startParams)>().deserialize(context, node["startParams"], startParams);
	}
}

ConfigNode ScriptState::toConfigNode(const EntitySerializationContext& context) const
{
	ConfigNode::MapType node;

	if (!context.matchType(EntitySerialization::makeMask(EntitySerialization::Type::Network))) {
		if (started) {
			node["started"] = started;
		}
		node["threads"] = ConfigNodeSerializer<decltype(threads)>().serialize(threads, context);
		node["nodeState"] = ConfigNodeSerializer<decltype(nodeState)>().serialize(nodeState, context);
		node["graphHash"] = Serializer::toBytes(graphHash);
		node["localVars"] = ConfigNodeSerializer<decltype(localVars)>().serialize(localVars, context);
		node["frameNumber"] = frameNumber;
	}

	if (!sharedVars.empty()) {
		node["sharedVars"] = ConfigNodeSerializer<decltype(sharedVars)>().serialize(sharedVars, context);
	}
	auto scriptName = scriptGraph ? scriptGraph->getAssetId() : "";
	if (!scriptName.isEmpty()) {
		node["script"] = std::move(scriptName);
	}
	if (persistAfterDone) {
		node["persistAfterDone"] = persistAfterDone;
	}
	if (!tags.empty()) {
		node["tags"] = tags;
	}
	if (!startParams.empty()) {
		node["startParams"] = ConfigNodeSerializer<decltype(startParams)>().serialize(startParams, context);
	}

	return node;
}

String ScriptState::getScriptId() const
{
	const auto* script = getScriptGraphPtr();
	return script ? script->getAssetId() : "";
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

void ScriptState::setTags(Vector<String> tags)
{
	this->tags = std::move(tags);
}

bool ScriptState::hasTag(const String& tag) const
{
	return std_ex::contains(tags, tag);
}

void ScriptState::setStartParams(Vector<ConfigNode> params)
{
	startParams = std::move(params);
}

gsl::span<const ConfigNode> ScriptState::getStartParams() const
{
	return startParams;
}

bool ScriptState::isDone() const
{
	return started && inbox.empty() && std::all_of(threads.begin(), threads.end(), [] (const ScriptStateThread& thread) { return thread.isWatcher(); });
}

bool ScriptState::isDead() const
{
	return isDone() && !persistAfterDone;
}

void ScriptState::setFrameFlag(bool flag)
{
	frameFlag = flag;
}

bool ScriptState::getFrameFlag() const
{
	return frameFlag;
}

bool ScriptState::hasThreadAt(GraphNodeId node) const
{
	for (const auto& thread: threads) {
		if (thread.getCurNode() == node) {
			return true;
		}
	}
	return false;
}

ScriptState::NodeIntrospection ScriptState::getNodeIntrospection(GraphNodeId nodeId) const
{
	NodeIntrospection result;
	result.state = NodeIntrospectionState::Unvisited;
	result.time = 0;

	const auto& state = nodeState.at(nodeId);
	result.activationTime = state.timeSinceStart;
	
	const auto& node = getScriptGraphPtr()->getNodes()[nodeId];
	if (node.getNodeType().getClassification() == ScriptNodeClassification::Variable || node.getNodeType().getClassification() == ScriptNodeClassification::Expression) {
		result.state = NodeIntrospectionState::Visited;
	} else {
		for (const auto& thread: threads) {
			if (thread.getCurNode() && nodeId == getScriptGraphPtr()->getNodeRoot(*thread.getCurNode())) {
				result.state = NodeIntrospectionState::Active;
				result.time = std::max(result.time, thread.getCurNodeTime());
			} else if (result.state == NodeIntrospectionState::Unvisited) {
				if (std_ex::contains_if(thread.getStack(), [&] (const auto& f) { return f.node == nodeId; })) {
					result.state = NodeIntrospectionState::Visited;
				}
			}
		}
	}

	return result;
}

void ScriptState::start(uint64_t hash)
{
	threads.clear();
	nodeCounters.clear();
	graphHash = hash;
	localVars.clear();
	sharedVars.clear();
	nodeState.clear();
	started = true;
}

void ScriptState::reset()
{
	threads.clear();
	nodeCounters.clear();
	started = false;
	graphHash = 0;
}

void ScriptState::prepareStates(const EntitySerializationContext& context, Time t)
{
	const auto& nodes = getScriptGraphPtr()->getNodes();
	if (needsStateLoading || nodeState.size() != nodes.size()) {
		nodeState.resize(nodes.size());
		for (size_t i = 0; i < nodes.size(); ++i) {
			ensureNodeLoaded(nodes[i], nodeState[i], context);
		}
		needsStateLoading = false;
	}

	for (auto& n: nodeState) {
		n.timeSinceStart += static_cast<float>(t);
	}
}

size_t& ScriptState::getNodeCounter(GraphNodeId node)
{
	return nodeCounters[node];
}

void ScriptState::updateDisplayOffset(Time t)
{
	Vector2f targetPos;
	size_t n = 0;
	for (const auto& t: threads) {
		const auto nodeId = t.getCurNode();
		if (nodeId) {
			const auto pos = getScriptGraphPtr()->getNodes().at(nodeId.value()).getPosition();
			targetPos += pos;
			++n;
		}
	}

	if (n > 0) {
		targetPos /= static_cast<float>(n);
	}

	displayOffset = damp(displayOffset, targetPos, 2.0f, static_cast<float>(t));
}

Vector2f ScriptState::getDisplayOffset() const
{
	return displayOffset;
}

bool ScriptState::operator==(const ScriptState& other) const
{
	return false;
}

bool ScriptState::operator!=(const ScriptState& other) const
{
	return true;
}

int ScriptState::getCurrentFrameNumber() const
{
	return frameNumber;
}

void ScriptState::incrementFrameNumber()
{
	++frameNumber;
}

void ScriptState::receiveMessage(ScriptMessage msg)
{
	const auto* script = getScriptGraphPtr();
	if (script && script->getMessageInboxId(msg.type.message, false)) {
		inbox.push_back(std::move(msg));
	}
}

void ScriptState::processMessages(Vector<GraphNodeId>& threadsToStart)
{
	std_ex::erase_if(inbox, [&] (ScriptMessage& msg)
	{
		return processMessage(msg, threadsToStart);
	});
}

bool ScriptState::processMessage(ScriptMessage& msg, Vector<GraphNodeId>& threadsToStart)
{
	if (const auto* scriptGraph = getScriptGraphPtr()) {
		if (const auto inboxId = scriptGraph->getMessageInboxId(msg.type.message)) {
			const auto& node = scriptGraph->getNodes().at(*inboxId);
			auto& state = getNodeState(*inboxId);
			assert(state.data);

			ScriptReceiveMessage receiveMsgNode;
			const bool accepted = receiveMsgNode.tryReceiveMessage(node, *dynamic_cast<ScriptReceiveMessageData*>(state.data), msg);
			if (accepted) {
				threadsToStart.push_back(*inboxId);
			}
			return accepted;
		}
	}

	// If we get here, then we can never accept this, consume it
	return true;
}

ScriptVariables& ScriptState::getLocalVariables()
{
	return localVars;
}

const ScriptVariables& ScriptState::getLocalVariables() const
{
	return localVars;
}

ScriptVariables& ScriptState::getSharedVariables()
{
	return sharedVars;
}

const ScriptVariables& ScriptState::getSharedVariables() const
{
	return sharedVars;
}

void ScriptState::offsetToNodeRange(Range<GraphNodeId> nodeRange)
{
	for (auto& t: threads) {
		t.offsetToNodeRange(nodeRange);
	}
	nodeState.erase(nodeState.begin(), nodeState.begin() + nodeRange.start);
	nodeState.resize(nodeRange.getLength());
}

ScriptState::NodeState& ScriptState::getNodeState(GraphNodeId nodeId)
{
	return nodeState.at(nodeId);
}

void ScriptState::startNode(const ScriptGraphNode& node, NodeState& state)
{
	assert(!state.hasPendingData);

	if (state.data) {
		auto& nodeType = node.getNodeType();
		nodeType.initData(*state.data, node, EntitySerializationContext(), ConfigNode());
	}
	
	state.timeSinceStart = 0;
}

void ScriptState::ensureNodeLoaded(const ScriptGraphNode& node, NodeState& state, const EntitySerializationContext& context)
{
	if (state.hasPendingData || !state.data) {
		auto& nodeType = node.getNodeType();
		auto data = nodeType.makeData();
		if (data) {
			nodeType.initData(*data, node, context, state.hasPendingData ? std::move(*state.pendingData) : ConfigNode());
			state.data = data.release();
			state.hasPendingData = false;
		}
	}
}

void ScriptState::finishNode(const ScriptGraphNode& node, NodeState& state, bool allThreadsDone)
{
	if (!state.hasPendingData && state.data && (allThreadsDone || !node.getNodeType().canKeepData())) {
		state.data->finishData();
	}
}

ConfigNode ConfigNodeSerializer<ScriptState>::serialize(const ScriptState& state, const EntitySerializationContext& context)
{
	return state.toConfigNode(context);
}

ScriptState ConfigNodeSerializer<ScriptState>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	return ScriptState(node, context);
}

void ConfigNodeSerializer<ScriptState>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptState& target)
{
	target.load(node, context);
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
