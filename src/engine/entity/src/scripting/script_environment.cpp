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
	serializationContext.entityContext = this;
}

void ScriptEnvironment::update(Time time, ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables)
{
	deltaTime = time;

	currentGraph = graphState.getScriptGraphPtr();
	if (!currentGraph) {
		throw Exception("Unable to update script state, script not set.", HalleyExceptions::Entity);
	}

	currentState = &graphState;
	currentEntityVariables = &entityVariables;
	currentGraph->assignTypes(nodeTypeCollection);
	currentEntity = curEntity;

	auto& threads = graphState.getThreads();

	const bool hashChanged = graphState.getGraphHash() != currentGraph->getHash();
	if (!graphState.hasStarted() || hashChanged) {
		if (hashChanged) {
			doTerminateState();
		}

		graphState.start(currentGraph->getHash());
		graphState.prepareStates(serializationContext, time);
		if (currentGraph->getStartNode()) {
			threads.push_back(startThread(ScriptStateThread(*currentGraph->getStartNode())));
		}
	} else {
		graphState.prepareStates(serializationContext, time);
	}

	Vector<ScriptNodeId> toStart;
	graphState.processMessages(time, toStart);
	for (const auto nodeId: toStart) {
		threads.push_back(startThread(ScriptStateThread(nodeId)));
	}

	// Allocate time for each thread
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

	graphState.updateDisplayOffset(time);
	graphState.incrementFrameNumber();

	currentGraph = nullptr;
	currentState = nullptr;
	currentEntityVariables = nullptr;
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

		// Update
		const auto result = nodeType.update(*this, static_cast<Time>(timeLeft), node, nodeState.data);
		thread.getCurNodeTime() += timeLeft;
		timeLeft -= static_cast<float>(result.timeElapsed);

		if (result.outputsCancelled != 0) {
			cancelOutputs(nodeId, result.outputsCancelled);
		}

		if (result.state == ScriptNodeExecutionState::Executing) {
			// Still running this node, suspend
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
			graphState.finishNode(node, nodeState, false);
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


void ScriptEnvironment::terminateState(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables)
{
	currentGraph = graphState.getScriptGraphPtr();
	if (!currentGraph) {
		throw Exception("Unable to terminate script state, script not set.", HalleyExceptions::Entity);
	}

	currentState = &graphState;
	currentEntityVariables = &entityVariables;
	currentGraph->assignTypes(nodeTypeCollection);
	currentEntity = curEntity;

	doTerminateState();

	currentGraph = nullptr;
	currentState = nullptr;
	currentEntityVariables = nullptr;
	currentEntity = EntityId();
}

void ScriptEnvironment::doTerminateState()
{
	for (auto& thread: currentState->getThreads()) {
		terminateThread(thread, false);
	}
	currentState->getThreads().clear();
}

ScriptStateThread ScriptEnvironment::startThread(ScriptStateThread thread)
{
	for (const auto s: thread.getStack()) {
		auto& nThreads = currentState->getNodeState(s.node).threadCount;
		assert(nThreads >= 1);
		++nThreads;
	}

	if (thread.getCurNode()) {
		const auto nodeId = thread.getCurNode().value();
		initNode(nodeId, currentState->getNodeState(nodeId));
	}

	return thread;
}

void ScriptEnvironment::addThread(ScriptStateThread thread, Vector<ScriptStateThread>& pending)
{
	pending.push_back(startThread(thread));
}

void ScriptEnvironment::advanceThread(ScriptStateThread& thread, OptionalLite<ScriptNodeId> node, ScriptPinId outputPin)
{
	if (node) {
		auto& state = currentState->getNodeState(node.value());
		if (state.threadCount == 0) {
			initNode(node.value(), state);
			thread.advanceToNode(node, outputPin);
			return;
		}
	}

	terminateThread(thread, true);
}

void ScriptEnvironment::initNode(ScriptNodeId nodeId, ScriptState::NodeState& nodeState)
{
	assert(nodeState.threadCount == 0);
	nodeState.threadCount++;
	currentState->startNode(currentGraph->getNodes()[nodeId], nodeState);
}

void ScriptEnvironment::forkThread(ScriptStateThread& thread, std::array<IScriptNodeType::OutputNode, 8> outputNodes, Vector<ScriptStateThread>& pendingThreads, size_t firstIdx)
{
	for (size_t j = firstIdx; j < outputNodes.size(); ++j) {
		if (outputNodes[j].dstNode && currentState->getNodeState(outputNodes[j].dstNode.value()).threadCount == 0) {
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
			terminateThread(other, false);
			break;
		}
	}

	if (wait) {
		thread.setMerging(true);
	}
}

void ScriptEnvironment::terminateThread(ScriptStateThread& thread, bool allowRollback)
{
	thread.advanceToNode({}, 0);
	
	auto& state = *currentState;
	
	auto& threadStack = thread.getStack();
	const auto n = static_cast<int>(threadStack.size());
	for (int i = n; --i >= 0;) {
		const auto nodeId = threadStack[i].node;
		const auto& node = currentGraph->getNodes()[nodeId];

		auto& nodeState = state.getNodeState(nodeId);

		if (allowRollback && i >= 1 && node.getNodeType().isStackRollbackPoint(*this, node, threadStack[i].pin, nodeState.data)) {
			threadStack.resize(i);
			thread.advanceToNode(nodeId, threadStack[i - 1].pin);
			return;
		}

		assert(nodeState.threadCount > 0);
		nodeState.threadCount--;
		if (nodeState.threadCount == 0) {
			if (node.getNodeType().hasDestructor()) {
				node.getNodeType().destructor(*this, node, nodeState.data);
			}
			state.finishNode(node, nodeState, true);
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
			terminateThread(thread, false);
		}
	}
}

EntityId ScriptEnvironment::getEntityIdFromUUID(const UUID& uuid) const
{
	auto e = world.findEntity(uuid, true);
	if (e) {
		return e->getEntityId();
	}
	return EntityId();
}

UUID ScriptEnvironment::getUUIDFromEntityId(EntityId id) const
{
	auto e = world.tryGetEntity(id);
	if (e.isValid()) {
		return e.getInstanceUUID();
	}
	return UUID();
}

EntityRef ScriptEnvironment::tryGetEntity(EntityId entityId)
{
	return world.tryGetEntity(entityId.isValid() ? entityId : currentEntity);
}

const ScriptGraph* ScriptEnvironment::getCurrentGraph() const
{
	return currentGraph;
}

size_t& ScriptEnvironment::getNodeCounter(ScriptNodeId nodeId)
{
	return currentState->getNodeCounter(nodeId);
}

void ScriptEnvironment::setDirection(EntityId entityId, const String& direction)
{
	if (auto* spriteAnimation = tryGetComponent<SpriteAnimationComponent>(entityId)) {
		spriteAnimation->player.setDirection(direction);
	}
}

void ScriptEnvironment::setInputEnabled(bool enabled)
{
	inputEnabled = enabled;
}

void ScriptEnvironment::setInputDevice(int idx, std::shared_ptr<InputDevice> input)
{
	inputDevices[idx] = std::move(input);
}

std::shared_ptr<InputDevice> ScriptEnvironment::getInputDevice(int idx) const
{
	if (!inputEnabled) {
		return {};
	}

	const auto iter = inputDevices.find(idx);
	if (iter != inputDevices.end()) {
		return iter->second;
	} else {
		return {};
	}
}

int ScriptEnvironment::getInputButtonByName(const String& name) const
{
	return static_cast<int>(fromString<DefaultInputButtons>(name));
}

void ScriptEnvironment::setHostNetworkAuthority(bool host)
{
	isHost = host;
}

bool ScriptEnvironment::hasNetworkAuthorityOver(EntityId id)
{
	return !world.isEntityNetworkRemote(tryGetEntity(id));
}

bool ScriptEnvironment::hasHostNetworkAuthority() const
{
	return isHost;
}

int ScriptEnvironment::getCurrentFrameNumber() const
{
	return currentState->getCurrentFrameNumber();
}

Time ScriptEnvironment::getDeltaTime() const
{
	return deltaTime;
}

EntityId ScriptEnvironment::getCurrentEntityId() const
{
	return currentEntity;
}

World& ScriptEnvironment::getWorld()
{
	return world;
}

Resources& ScriptEnvironment::getResources()
{
	return resources;
}

void ScriptEnvironment::sendScriptMessage(EntityId dstEntity, ScriptMessage message)
{
	if (!dstEntity.isValid()) {
		dstEntity = currentEntity;
	}

	scriptOutbox.emplace_back(dstEntity, std::move(message));
}

void ScriptEnvironment::sendEntityMessage(EntityMessageData message)
{
	Expects(!message.messageName.isEmpty());

	if (!message.targetEntity.isValid()) {
		message.targetEntity = currentEntity;
	}

	entityOutbox.emplace_back(std::move(message));
}

void ScriptEnvironment::sendSystemMessage(SystemMessageData message)
{
	auto msg = world.deserializeSystemMessage(message.messageName, message.messageData);
	const auto dst = msg->getMessageDestination();
	const auto id = msg->getId();

	SystemMessageContext context;
	context.msg = std::move(msg);
	context.msgId = id;
	context.remote = false;

	world.sendSystemMessage(std::move(context), message.targetSystem, dst);
}

Vector<std::pair<EntityId, ScriptMessage>> ScriptEnvironment::getOutboundScriptMessages()
{
	return std::move(scriptOutbox);
}

Vector<ScriptEnvironment::EntityMessageData> ScriptEnvironment::getOutboundEntityMessages()
{
	return std::move(entityOutbox);
}

std::shared_ptr<UIWidget> ScriptEnvironment::createInWorldUI(const String& ui, Vector2f offset, Vector2f alignment, EntityId entityId)
{
	return {};
}

std::shared_ptr<UIWidget> ScriptEnvironment::createModalUI(const String& ui)
{
	return {};
}

IScriptStateData* ScriptEnvironment::getNodeData(ScriptNodeId nodeId)
{
	return currentState->getNodeState(nodeId).data;
}

void ScriptEnvironment::assignTypes(const ScriptGraph& graph)
{
	graph.assignTypes(nodeTypeCollection);
}

void ScriptEnvironment::postAudioEvent(const String& id, EntityId entityId)
{
	if (const auto* audioSource = tryGetComponent<AudioSourceComponent>(entityId)) {
		api.audio->postEvent(id, audioSource->emitter);
	} else {
		api.audio->postEvent(id);
	}
}

ScriptVariables& ScriptEnvironment::getVariables(ScriptVariableScope scope)
{
	switch (scope) {
	case ScriptVariableScope::Local:
		return currentState->getLocalVariables();
	case ScriptVariableScope::Shared:
		return currentState->getSharedVariables();
	case ScriptVariableScope::Entity:
		return *currentEntityVariables;
	default:
		throw Exception("Variable type " + toString(scope) + " not implemented", HalleyExceptions::Entity);
	}
}

const ScriptVariables& ScriptEnvironment::getVariables(ScriptVariableScope scope) const
{
	switch (scope) {
	case ScriptVariableScope::Local:
		return currentState->getLocalVariables();
	case ScriptVariableScope::Shared:
		return currentState->getSharedVariables();
	case ScriptVariableScope::Entity:
		return *currentEntityVariables;
	default:
		throw Exception("Variable type " + toString(scope) + " not implemented", HalleyExceptions::Entity);
	}
}
