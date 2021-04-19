#include "scripting/script_environment.h"

#include "world.h"
#include "halley/support/logger.h"
#include "nodes/script_play_animation.h"
#include "nodes/script_wait.h"
#include "scripting/script_graph.h"
#include "scripting/script_state.h"

using namespace Halley;

ScriptEnvironment::ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources)
	: api(api)
	, world(world)
	, resources(resources)
{
	addBasicScriptNodes();
}

void ScriptEnvironment::addBasicScriptNodes()
{
	addScriptNode(std::make_unique<ScriptWait>());
	addScriptNode(std::make_unique<ScriptPlayAnimation>());
}

void ScriptEnvironment::addScriptNode(std::unique_ptr<IScriptNodeType> nodeType)
{
	auto name = nodeType->getName();
	nodeTypes[std::move(name)] = std::move(nodeType);
}

void ScriptEnvironment::update(Time time, const ScriptGraph& graph, ScriptState& state)
{
	if (!state.hasStarted() || state.getGraphHash() != graph.getHash()) {
		state.start(graph.getStartNode(), graph.getHash());
	}

	// Allocate time for each thread
	auto& threads = state.getThreads();
	if (threads.empty()) {
		return;
	}
	for (auto& thread: threads) {
		thread.getTimeSlice() = time;
	}
	
	for (size_t i = 0; i < threads.size(); ++i) {
		auto& thread = threads[i];
		Time& timeLeft = thread.getTimeSlice();

		while (timeLeft > 0 && thread.getCurNode()) {
			const auto& node = graph.getNodes().at(thread.getCurNode().value());

			// Start node if not done yet
			if (!thread.isNodeStarted()) {
				thread.startNode(makeNodeData(node));
			}

			// Update
			auto [timeConsumed, state] = updateNode(time, node, thread.getCurData());
			timeLeft -= timeConsumed;

			if (state == ScriptNodeExecutionState::Terminate) {
				// Terminate script
				threads.clear();
				break;
			} else if (state == ScriptNodeExecutionState::Done) {
				// Proceed to next node(s)
				thread.finishNode();
				const auto& outputs = node.getOutputs();
				if (outputs.empty()) {
					// Nothing follows this, terminate thread
					thread.advanceToNode({});
				} else {
					// Direct sequel
					thread.advanceToNode(outputs[0].nodeId);
					
					if (outputs.size() > 1)	{
						// Spawn others as new threads
						for (size_t j = 1; j < outputs.size(); ++j) {
							auto& newThread = threads.emplace_back(outputs[j].nodeId);
							newThread.getTimeSlice() = timeLeft;
						}
					}
				}
			}
		}
	}

	// Remove stopped threads
	threads.erase(std::remove_if(threads.begin(), threads.end(), [&] (const ScriptStateThread& thread) { return !thread.getCurNode(); }), threads.end());
}

EntityRef ScriptEnvironment::getEntity(EntityId entityId)
{
	return world.getEntity(entityId);
}

IScriptNodeType::Result ScriptEnvironment::updateNode(Time time, const ScriptGraphNode& node, IScriptStateData* curData)
{
	const auto iter = nodeTypes.find(node.getType());
	if (iter == nodeTypes.end()) {
		Logger::logError("Unknown node type: \"" + node.getType() + "\"");
		return {0, ScriptNodeExecutionState::Done};
	}
	return iter->second->update(*this, time, node, curData);
}

std::unique_ptr<IScriptStateData> ScriptEnvironment::makeNodeData(const ScriptGraphNode& node)
{
	const auto iter = nodeTypes.find(node.getType());
	if (iter == nodeTypes.end()) {
		Logger::logError("Unknown node type: \"" + node.getType() + "\"");
		return {};
	}
	const auto& nodeType = iter->second;
	
	auto result = nodeType->makeData();
	if (result) {
		nodeType->initData(*result, node);
	}
	return result;
}
