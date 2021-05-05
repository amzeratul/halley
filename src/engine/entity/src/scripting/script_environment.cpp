#include "scripting/script_environment.h"

#include "world.h"
#include "halley/support/logger.h"
#include "scripting/script_graph.h"
#include "scripting/script_state.h"

using namespace Halley;

ScriptEnvironment::ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources, const ScriptNodeTypeCollection& nodeTypeCollection)
	: api(api)
	, world(world)
	, resources(resources)
	, nodeTypeCollection(nodeTypeCollection)
{
}

void ScriptEnvironment::update(Time time, const ScriptGraph& graph, ScriptState& graphState)
{
	if (!graphState.hasStarted() || graphState.getGraphHash() != graph.getHash()) {
		graphState.start(graph.getStartNode(), graph.getHash());
	}

	// Allocate time for each thread
	auto& threads = graphState.getThreads();
	if (threads.empty()) {
		return;
	}
	for (auto& thread: threads) {
		thread.getTimeSlice() = time;
	}
	
	for (size_t i = 0; i < threads.size(); ++i) {
		auto& thread = threads[i];
		Time& timeLeft = thread.getTimeSlice();
		bool suspended = false;

		while (!suspended && timeLeft > 0 && thread.getCurNode()) {
			const auto& node = graph.getNodes().at(thread.getCurNode().value());

			// Start node if not done yet
			if (!thread.isNodeStarted()) {
				thread.startNode(makeNodeData(node));
			}

			// Update
			const auto result = updateNode(time, node, thread.getCurData());
			timeLeft -= result.timeElapsed;

			if (result.state == ScriptNodeExecutionState::Done) {
				// Proceed to next node(s)
				thread.finishNode();

				size_t nOutputsFound = 0;
				for (size_t j = 0; j < node.getPins().size(); ++j) {
					const auto& output = node.getPins()[j];
					const bool outputActive = (result.outputsActive & (1 << j)) != 0;
					
					if (outputActive && output.dstNode) {
						if (nOutputsFound == 0) {
							// Direct sequel
							thread.advanceToNode(output.dstNode.value());
						} else {
							// Spawn as new thread
							auto& newThread = threads.emplace_back(output.dstNode.value());
							newThread.getTimeSlice() = timeLeft;
						}

						++nOutputsFound;
					}
				}
				if (nOutputsFound == 0) {
					// Nothing follows this, terminate thread
					thread.advanceToNode({});
				}
			} else if (result.state == ScriptNodeExecutionState::Executing) {
				// Still running this node, suspend
				suspended = true;
			} else if (result.state == ScriptNodeExecutionState::Terminate) {
				// Terminate script
				threads.clear();
				break;
			} else if (result.state == ScriptNodeExecutionState::Restart) {
				// Restart script
				threads.clear();
				graphState.start(graph.getStartNode(), graph.getHash());
				break;
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
	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		Logger::logError("Unknown node type: \"" + node.getType() + "\"");
		return IScriptNodeType::Result(ScriptNodeExecutionState::Done);
	}
	return nodeType->update(*this, time, node, curData);
}

std::unique_ptr<IScriptStateData> ScriptEnvironment::makeNodeData(const ScriptGraphNode& node)
{
	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		Logger::logError("Unknown node type: \"" + node.getType() + "\"");
		return {};
	}
	
	auto result = nodeType->makeData();
	if (result) {
		nodeType->initData(*result, node);
	}
	return result;
}
