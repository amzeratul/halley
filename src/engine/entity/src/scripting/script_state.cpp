#include "scripting/script_state.h"
using namespace Halley;

ScriptStateThread::ScriptStateThread()
{
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
	Expects(!nodeStarted);
	curNode = node;
}

ScriptState::ScriptState()
{}

ScriptState::ScriptState(const ConfigNode& node)
{
	// TODO
}

ConfigNode ScriptState::toConfigNode() const
{
	ConfigNode::MapType result;
	// TODO
	return result;
}

void ScriptState::start(OptionalLite<uint32_t> startNode, uint64_t hash)
{
	threads.clear();
	if (startNode) {
		threads.emplace_back(startNode.value());
	}
	graphHash = hash;
	started = true;
}


ConfigNode ConfigNodeSerializer<ScriptState>::serialize(const ScriptState& state, const ConfigNodeSerializationContext& context)
{
	return state.toConfigNode();
}

ScriptState ConfigNodeSerializer<ScriptState>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptState(node);
}
