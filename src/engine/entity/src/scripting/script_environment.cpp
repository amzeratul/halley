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
	currentGraph = &graph;
	graph.assignTypes(nodeTypeCollection);
	
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
			// Get node type
			const auto& node = graph.getNodes().at(thread.getCurNode().value());
			const auto& nodeType = node.getNodeType();
			
			// Start node if not done yet
			if (!thread.isNodeStarted()) {
				thread.startNode(makeNodeData(nodeType, node));
			}

			// Update
			const auto result = nodeType.update(*this, time, node, thread.getCurData());

			if (result.state == ScriptNodeExecutionState::Done) {
				// Proceed to next node(s)
				thread.finishNode();

				size_t nOutputsFound = 0;
				size_t curOutputPin = 0;
				const auto& pinConfig = nodeType.getPinConfiguration();
				for (size_t j = 0; j < pinConfig.size(); ++j) {
					if (pinConfig[j].type == ScriptNodeElementType::FlowPin && pinConfig[j].direction == ScriptNodePinDirection::Output) {
						const bool outputActive = (result.outputsActive & (1 << curOutputPin)) != 0;
						if (outputActive) {
							const auto& output = node.getPin(j);
							if (output.dstNode) {
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

						++curOutputPin;
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

	currentGraph = nullptr;
}

EntityRef ScriptEnvironment::getEntity(EntityId entityId)
{
	return world.getEntity(entityId);
}

const ScriptGraph* ScriptEnvironment::getCurrentGraph() const
{
	return currentGraph;
}

std::unique_ptr<IScriptStateData> ScriptEnvironment::makeNodeData(const IScriptNodeType& nodeType, const ScriptGraphNode& node)
{
	auto result = nodeType.makeData();
	if (result) {
		nodeType.initData(*result, node);
	}
	return result;
}
