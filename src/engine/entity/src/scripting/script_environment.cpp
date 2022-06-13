#include "scripting/script_environment.h"


#include "world.h"
#include "halley/core/api/halley_api.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "scripting/script_graph.h"
#include "scripting/script_state.h"
#include "halley/core/api/audio_api.h"
#include "halley/audio/audio_event.h"

#include "halley/core/graphics/sprite/animation_player.h"

#include <components/audio_source_component.h>
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
	graphState.ensureReady();

	// Allocate time for each thread
	auto& threads = graphState.getThreads();
	for (auto& thread: threads) {
		thread.getTimeSlice() = static_cast<float>(time);
	}
	
	// Update all threads
	Vector<ScriptStateThread> pendingThreads;
	for (size_t i = 0; i < threads.size(); ++i) {
		updateThread(graphState, threads[i], pendingThreads);

		for (auto& t: pendingThreads) {
			threads.push_back(std::move(t));
		}
		pendingThreads.clear();
	}
	removeStoppedThreads();

	// Clean up if done
	if (graphState.isDone()) {
		doTerminateState();
	}

	currentGraph = nullptr;
	currentState = nullptr;
	currentEntity = EntityId();
}

void ScriptEnvironment::updateThread(ScriptState& graphState, ScriptStateThread& thread, Vector<ScriptStateThread>& pendingThreads)
{
	float& timeLeft = thread.getTimeSlice();

	while (timeLeft > 0 && thread.isRunning()) {
		// Get node type
		const auto nodeId = thread.getCurNode().value();
		const auto& node = currentGraph->getNodes().at(nodeId);
		const auto& nodeType = node.getNodeType();
		auto& nodeState = graphState.getNodeState(nodeId);
		graphState.startNode(node, nodeState);

		// Update
		const auto result = nodeType.update(*this, static_cast<Time>(timeLeft), node, nodeState.data);
		timeLeft -= static_cast<float>(result.timeElapsed);

		if (result.outputsCancelled != 0) {
			cancelOutputs(nodeId, result.outputsCancelled);
		}

		if (result.state == ScriptNodeExecutionState::Executing) {
			// Still running this node, suspend
			thread.getCurNodeTime() += timeLeft;
			timeLeft = 0;
		} else if (result.state == ScriptNodeExecutionState::Fork || result.state == ScriptNodeExecutionState::ForkAndConvertToWatcher) {
			forkThread(thread, nodeType.getOutputNodes(node, result.outputsActive), pendingThreads);
			if (result.state == ScriptNodeExecutionState::ForkAndConvertToWatcher) {
				thread.setWatcher(true);
			}
		} else if (result.state == ScriptNodeExecutionState::MergeAndWait) {
			mergeThread(thread, true);
		} else {
			// Node ended
			graphState.finishNode(node, nodeState);
			thread.setWatcher(false);

			if (result.state == ScriptNodeExecutionState::Done || result.state == ScriptNodeExecutionState::MergeAndContinue) {
				if (result.state == ScriptNodeExecutionState::MergeAndContinue) {
					mergeThread(thread, false);
				}

				auto outputNodes = nodeType.getOutputNodes(node, result.outputsActive);
				forkThread(thread, outputNodes, pendingThreads, 1);
				advanceThread(thread, outputNodes[0].dstNode, outputNodes[0].outputPin);
			} else if (result.state == ScriptNodeExecutionState::Terminate) {
				doTerminateState();
				break;
			} else if (result.state == ScriptNodeExecutionState::Restart) {
				doTerminateState();
				graphState.reset();
				break;
			}
		}
	}
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

void ScriptEnvironment::addThread(ScriptStateThread thread, Vector<ScriptStateThread>& pending)
{
	for (const auto s: thread.getStack()) {
		auto& nThreads = currentState->getNodeState(s.node).threadCount;
		assert(nThreads >= 1);
		++nThreads;
	}

	pending.push_back(std::move(thread));
}

void ScriptEnvironment::advanceThread(ScriptStateThread& thread, OptionalLite<ScriptNodeId> node, ScriptPinId outputPin)
{
	if (node) {
		thread.advanceToNode(node, outputPin);
	} else {
		terminateThread(thread);
	}
}

void ScriptEnvironment::forkThread(ScriptStateThread& thread, std::array<IScriptNodeType::OutputNode, 8> outputNodes, Vector<ScriptStateThread>& pendingThreads, size_t firstIdx)
{
	for (size_t j = firstIdx; j < outputNodes.size(); ++j) {
		if (outputNodes[j].dstNode) {
			addThread(thread.fork(outputNodes[j].dstNode.value(), outputNodes[j].outputPin), pendingThreads);
		}
	}
}

void ScriptEnvironment::mergeThread(ScriptStateThread& thread, bool wait)
{
	for (auto& other: currentState->getThreads()) {
		if (&thread != &other && other.isMerging() && other.getCurNode() == thread.getCurNode()) {
			thread.merge(other);
			other.setMerging(false);
			terminateThread(other);
			break;
		}
	}

	if (wait) {
		thread.setMerging(true);
	}
}

void ScriptEnvironment::terminateThread(ScriptStateThread& thread)
{
	thread.advanceToNode({}, 0);

	auto& state = *currentState;
	
	auto& threadStack = thread.getStack();
	const auto n = static_cast<int>(threadStack.size());
	for (int i = n; --i >= 0;) {
		const auto nodeId = threadStack[i].node;
		const auto& node = currentGraph->getNodes()[nodeId];

		auto& nodeState = state.getNodeState(nodeId);
		state.ensureNodeLoaded(node, nodeState);
		nodeState.threadCount--;
		if (nodeState.threadCount == 0) {
			node.getNodeType().destructor(*this, node, nodeState.data);
			state.finishNode(node, nodeState);
		}
	}
	threadStack.clear();
}

void ScriptEnvironment::removeStoppedThreads()
{
	std_ex::erase_if(currentState->getThreads(), [&] (const ScriptStateThread& thread) { return !thread.getCurNode(); });
}

void ScriptEnvironment::cancelOutputs(ScriptNodeId nodeId, uint8_t cancelMask)
{
	if (cancelMask == 0xFF) {
		abortCodePath(nodeId, {});
	} else {
		for (uint8_t i = 0; i < 8; ++i) {
			if ((cancelMask & (1 << i)) != 0) {
				auto& node = currentGraph->getNodes()[nodeId];
				const auto pinIdx = node.getNodeType().getNthOutputPinIdx(node, i);
				assert(node.getPinType(pinIdx).isCancellable);
				abortCodePath(nodeId, pinIdx);
			}
		}
	}
}

void ScriptEnvironment::abortCodePath(ScriptNodeId node, std::optional<ScriptPinId> outputPin)
{
	Expects(currentState != nullptr);

	for (auto& thread: currentState->getThreads()) {
		if (thread.stackGoesThrough(node, outputPin)) {
			terminateThread(thread);
		}
	}
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
	if (auto* spriteAnimation = tryGetComponent<SpriteAnimationComponent>(entityId)) {
		spriteAnimation->player.setDirection(direction);
	}
}

void ScriptEnvironment::setInputDevice(int idx, std::shared_ptr<InputDevice> input)
{
	inputDevices[idx] = std::move(input);
}

std::shared_ptr<InputDevice> ScriptEnvironment::getInputDevice(int idx) const
{
	const auto iter = inputDevices.find(idx);
	if (iter != inputDevices.end()) {
		return iter->second;
	} else {
		return {};
	}
}

void ScriptEnvironment::postAudioEvent(const String& id, EntityId entityId)
{
	if (const auto* audioSource = tryGetComponent<AudioSourceComponent>(entityId)) {
		api.audio->postEvent(id, audioSource->emitter);
	} else {
		api.audio->postEvent(id);
	}
}
