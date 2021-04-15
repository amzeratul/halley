#include "halley/core/scripting/script_environment.h"
#include "scripting/script_graph.h"
#include "scripting/script_state.h"

using namespace Halley;

ScriptEnvironment::ScriptEnvironment()
{
}

void ScriptEnvironment::update(Time time, const ScriptGraph& graph, ScriptState& state)
{
	if (!state.hasStarted() || state.getGraphHash() != graph.getHash()) {
		state.start(graph.getStartNode(), graph.getHash());
	}

	// Allocate time for each thread
	auto& threads = state.getThreads();
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
				thread.startNode(makeNodeData(node.getType()));
			}

			// Update
			auto [timeConsumed, done] = updateNode(time, node, thread.getCurData());
			timeLeft -= timeConsumed;

			// Proceed to next node(s)
			if (done) {
				thread.finishNode();
				const auto& outputs = node.getOutput();
				if (outputs.empty()) {
					// Nothing follows this, terminate thread
					thread.advanceToNode({});
				} else {
					// Direct sequel
					thread.advanceToNode(outputs[0]);
					
					if (outputs.size() > 1)	{
						// Spawn others as new threads
						for (size_t j = 1; j < outputs.size(); ++j) {
							auto& newThread = threads.emplace_back(outputs[j]);
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

void ScriptEnvironment::addScriptNode(std::unique_ptr<IScriptNodeType> nodeType)
{
	auto name = nodeType->getName();
	nodeTypes[std::move(name)] = std::move(nodeType);
}

IScriptNodeType::Result ScriptEnvironment::updateNode(Time time, const ScriptGraphNode& node, IScriptStateData* curData)
{
	const auto iter = nodeTypes.find(node.getType());
	if (iter == nodeTypes.end()) {
		return {0, true};
	}
	return iter->second->update(time, node.getSettings(), curData);
}

std::unique_ptr<IScriptStateData> ScriptEnvironment::makeNodeData(const String& type)
{
	const auto iter = nodeTypes.find(type);
	if (iter == nodeTypes.end()) {
		return {};
	}
	return iter->second->makeData();
}
