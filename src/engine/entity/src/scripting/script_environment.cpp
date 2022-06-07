#include "scripting/script_environment.h"
#include "world.h"
#include "halley/core/api/halley_api.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "scripting/script_graph.h"
#include "scripting/script_state.h"

#include "halley/core/graphics/sprite/animation_player.h"
#include <components/sprite_animation_component.h>

using namespace Halley;

ScriptEnvironment::ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources, const ScriptNodeTypeCollection& nodeTypeCollection)
	: api(api)
	, world(world)
	, resources(resources)
	, nodeTypeCollection(nodeTypeCollection)
{
}

void ScriptEnvironment::update(Time time, ScriptState& graphState, EntityId curEntity)
{
	currentGraph = graphState.getScriptGraphPtr();
	if (!currentGraph) {
		throw Exception("Unable to update script state, script not set.", HalleyExceptions::Entity);
	}

	currentState = &graphState;
	currentGraph->assignTypes(nodeTypeCollection);
	currentEntity = curEntity;
	
	if (!graphState.hasStarted() || graphState.getGraphHash() != currentGraph->getHash()) {
		graphState.start(currentGraph->getStartNode(), currentGraph->getHash());
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
			const auto& node = currentGraph->getNodes().at(nodeId);
			const auto& nodeType = node.getNodeType();
			auto& nodeState = graphState.getNodeState(nodeId);
			
			// Start node if not done yet
			if (nodeState.threadCount == 0) {
				graphState.startNode(node, nodeState);
			}
			graphState.onNodeStarted(nodeId);
			nodeState.threadCount++;

			// Update
			const auto result = nodeType.update(*this, time, node, nodeState.data);

			if (result.state == ScriptNodeExecutionState::Executing) {
				// Still running this node, suspend
				suspended = true;
			} else {
				// Node ended
				graphState.onNodeEnded(nodeId);
				graphState.finishNode(node, nodeState);

				if (result.state == ScriptNodeExecutionState::Done) {
					// Proceed to next node(s)
					auto outputNodes = nodeType.getOutputNodes(node, result.outputsActive);
					if (!outputNodes[0]) {
						terminateThread(thread);
					} else {
						thread.advanceToNode(outputNodes[0]);
						for (size_t j = 1; j < outputNodes.size(); ++j) {
							if (outputNodes[j]) {
								auto& newThread = threads.emplace_back(outputNodes[j].value());
								newThread.getTimeSlice() = timeLeft;
							}
						}
					}
				} else if (result.state == ScriptNodeExecutionState::Terminate) {
					// Terminate script
					doTerminateState();
					break;
				} else if (result.state == ScriptNodeExecutionState::Restart) {
					// Restart script
					doTerminateState();
					graphState.reset();
					break;
				} else if (result.state == ScriptNodeExecutionState::Merged) {
					// Merged thread
					terminateThread(thread);
				}
			}
		}
	}

	// Remove stopped threads
	std_ex::erase_if(threads, [&] (const ScriptStateThread& thread) { return !thread.getCurNode(); });

	graphState.updateIntrospection(time);

	currentGraph = nullptr;
	currentState = nullptr;
	currentEntity = EntityId();
}

void ScriptEnvironment::terminateState(ScriptState& graphState, EntityId curEntity)
{
	currentGraph = graphState.getScriptGraphPtr();
	if (!currentGraph) {
		throw Exception("Unable to terminate script state, script not set.", HalleyExceptions::Entity);
	}

	currentState = &graphState;
	currentGraph->assignTypes(nodeTypeCollection);
	currentEntity = curEntity;

	doTerminateState();

	currentGraph = nullptr;
	currentState = nullptr;
	currentEntity = EntityId();
}

void ScriptEnvironment::doTerminateState()
{
	for (auto& thread: currentState->getThreads()) {
		terminateThread(thread);
	}
	currentState->getThreads().clear();
}

void ScriptEnvironment::terminateThread(ScriptStateThread& thread)
{
	thread.advanceToNode({});

	auto& state = *currentState;
	
	auto& currentStack = thread.getStack();
	const auto n = static_cast<int>(currentStack.size());
	for (int i = n; --i >= 0;) {
		const auto nodeId = currentStack[i];
		const auto& node = currentGraph->getNodes()[nodeId];

		auto& nodeState = state.getNodeState(nodeId);
		nodeState.threadCount--;
		if (nodeState.threadCount == 0) {
			node.getNodeType().destructor(*this, node);
			state.finishNode(node, nodeState);
		}
	}

	currentStack.clear();
}

EntityRef ScriptEnvironment::tryGetEntity(EntityId entityId)
{
	return world.tryGetEntity(entityId.isValid() ? entityId : currentEntity);
}

const ScriptGraph* ScriptEnvironment::getCurrentGraph() const
{
	return currentGraph;
}

size_t& ScriptEnvironment::getNodeCounter(uint32_t nodeId)
{
	return currentState->getNodeCounter(nodeId);
}

void ScriptEnvironment::playMusic(const String& music, float fadeTime)
{
	api.audio->playMusic(music, 0, fadeTime);
}

void ScriptEnvironment::stopMusic(float fadeTime)
{
	api.audio->stopMusic(0, fadeTime);
}

ConfigNode ScriptEnvironment::getVariable(const String& variable)
{
	return currentState->getVariable(variable);
}

void ScriptEnvironment::setVariable(const String& variable, ConfigNode data)
{
	currentState->setVariable(variable, std::move(data));
}

void ScriptEnvironment::setDirection(EntityId entityId, const String& direction)
{
	auto entity = tryGetEntity(entityId);
	if (entity.isValid()) {
		auto* spriteAnimation = entity.tryGetComponent<SpriteAnimationComponent>();
		if (spriteAnimation) {
			spriteAnimation->player.setDirection(direction);
		}
	}
}
