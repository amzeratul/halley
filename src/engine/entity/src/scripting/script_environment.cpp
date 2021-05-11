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
	currentState = &graphState;
	graph.assignTypes(nodeTypeCollection);
	
	if (!graphState.hasStarted() || graphState.getGraphHash() != graph.getHash()) {
		graphState.start(graph.getStartNode(), graph.getHash());
	}

	// Allocate time for each thread
	auto& threads = graphState.getThreads();
	for (auto& thread: threads) {
		thread.getTimeSlice() = static_cast<float>(time);
	}
	
	for (size_t i = 0; i < threads.size(); ++i) {
		auto& thread = threads[i];
		float& timeLeft = thread.getTimeSlice();
		bool suspended = false;

		while (!suspended && timeLeft > 0 && thread.getCurNode()) {
			// Get node type
			const auto nodeId = thread.getCurNode().value();
			const auto& node = graph.getNodes().at(nodeId);
			const auto& nodeType = node.getNodeType();
			
			// Start node if not done yet
			if (!thread.isNodeStarted()) {
				thread.startNode(makeNodeData(nodeType, node, thread.getPendingNodeData()));
				graphState.onNodeStarted(nodeId);
			}

			// Update
			const auto result = nodeType.update(*this, time, node, thread.getCurData());

			if (result.state == ScriptNodeExecutionState::Done) {
				// Proceed to next node(s)
				thread.finishNode();
				graphState.onNodeEnded(nodeId);

				auto outputNodes = nodeType.getOutputNodes(node, result.outputsActive);
				thread.advanceToNode(outputNodes[0]);
				for (size_t j = 1; j < outputNodes.size(); ++j) {
					if (outputNodes[j]) {
						auto& newThread = threads.emplace_back(outputNodes[j].value());
						newThread.getTimeSlice() = timeLeft;
					}
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
				graphState.reset();
				break;
			} else if (result.state == ScriptNodeExecutionState::Merged) {
				// Merged thread
				thread.advanceToNode({});
				break;
			}
		}
	}

	// Remove stopped threads
	threads.erase(std::remove_if(threads.begin(), threads.end(), [&] (const ScriptStateThread& thread) { return !thread.getCurNode(); }), threads.end());

	graphState.updateIntrospection(time);

	currentGraph = nullptr;
	currentState = nullptr;
}

EntityRef ScriptEnvironment::tryGetEntity(EntityId entityId)
{
	return world.tryGetEntity(entityId);
}

const ScriptGraph* ScriptEnvironment::getCurrentGraph() const
{
	return currentGraph;
}

size_t& ScriptEnvironment::getNodeCounter(uint32_t nodeId)
{
	return currentState->getNodeCounter(nodeId);
}

std::unique_ptr<IScriptStateData> ScriptEnvironment::makeNodeData(const IScriptNodeType& nodeType, const ScriptGraphNode& node, const ConfigNode& nodeData)
{
	auto result = nodeType.makeData();
	if (result) {
		nodeType.initData(*result, node, nodeData);
	}
	return result;
}
