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
#include <components/scriptable_component.h>
#include <components/sprite_animation_component.h>

#include "halley/support/profiler.h"

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

	ProfilerEvent event(ProfilerEventType::ScriptUpdate, currentGraph->getAssetId());

	currentState = &graphState;
	currentEntityVariables = &entityVariables;
	currentGraph->assignTypes(nodeTypeCollection);
	currentEntity = curEntity;

	auto& threads = graphState.getThreads();

	const bool hashChanged = graphState.getGraphHash() != currentGraph->getHash();
	if (!graphState.hasStarted() || hashChanged) {
		graphState.start(currentGraph->getHash());
		graphState.prepareStates(serializationContext, time);
		if (currentGraph->getStartNode()) {
			threads.push_back(startThread(ScriptStateThread(*currentGraph->getStartNode(), 0)));
		}
	} else {
		graphState.prepareStates(serializationContext, time);
	}
	
	processMessages(time, threads);

	// Allocate time for each thread
	for (auto& thread: threads) {
		thread.getTimeSlice() = static_cast<float>(time);
	}
	
	// Update all threads
	Vector<ScriptStateThread> pendingThreads;
	for (size_t i = 0; i < threads.size(); ++i) {
		const bool running = updateThread(graphState, threads[i], pendingThreads);
		if (running) {
			processMessages(threads[i].getTimeSlice(), pendingThreads);

			for (auto& t: pendingThreads) {
				threads.push_back(std::move(t));
			}
			pendingThreads.clear();
		}
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

bool ScriptEnvironment::updateThread(ScriptState& graphState, ScriptStateThread& thread, Vector<ScriptStateThread>& pendingThreads)
{
	float& timeLeft = thread.getTimeSlice();

	while (timeLeft > 0 && thread.isRunning()) {
		// Get node type
		const auto nodeId = thread.getCurNode().value();
		const auto& node = currentGraph->getNodes().at(nodeId);
		const auto& nodeType = node.getNodeType();
		auto& nodeState = graphState.getNodeState(nodeId);
		currentInputPin = thread.getCurInputPin();

		// Dead watcher
		if (nodeState.threadCount == 0 && thread.isWatcher()) {
			terminateThread(thread, false);
			continue;
		}
		
		// Update
		const auto result = nodeType.update(*this, static_cast<Time>(timeLeft), node, nodeState.data);
		thread.getCurNodeTime() += timeLeft;
		timeLeft -= static_cast<float>(result.timeElapsed);

		if (result.state == ScriptNodeExecutionState::Executing) {
			// Still running this node, suspend
			timeLeft = 0;
		} else if (result.state == ScriptNodeExecutionState::Fork || result.state == ScriptNodeExecutionState::ForkAndConvertToWatcher) {
			forkThread(thread, nodeType.getOutputNodes(node, result.outputsActive), pendingThreads);
			if (result.state == ScriptNodeExecutionState::ForkAndConvertToWatcher) {
				setWatcher(thread, true);
			}
		} else if (result.state == ScriptNodeExecutionState::MergeAndWait) {
			mergeThread(thread, true);
		} else {
			// Node ended
			graphState.finishNode(node, nodeState, false);
			setWatcher(thread, false);

			if (result.state == ScriptNodeExecutionState::Done || result.state == ScriptNodeExecutionState::MergeAndContinue) {
				if (result.state == ScriptNodeExecutionState::MergeAndContinue) {
					mergeThread(thread, false);
				}

				auto outputNodes = nodeType.getOutputNodes(node, result.outputsActive);
				forkThread(thread, outputNodes, pendingThreads, 1);
				advanceThread(thread, outputNodes[0].dstNode, outputNodes[0].outputPin, outputNodes[0].inputPin);
			} else if (result.state == ScriptNodeExecutionState::Terminate) {
				doTerminateState();
				return false;
			} else if (result.state == ScriptNodeExecutionState::Restart) {
				doTerminateState();
				graphState.reset();
				return false;
			} else if (result.state == ScriptNodeExecutionState::Call) {
				callFunction(thread);
			} else if (result.state == ScriptNodeExecutionState::Return) {
				returnFromFunction(thread, result.outputsActive, pendingThreads);
			}
		}

		if (result.outputsCancelled != 0) {
			cancelOutputs(nodeId, result.outputsCancelled);
		}
	}
	return true;
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

ConfigNode ScriptEnvironment::readNodeElementDevConData(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables, GraphNodeId nodeId, GraphPinId pinId)
{
	currentGraph = graphState.getScriptGraphPtr();
	currentState = &graphState;
	currentEntityVariables = &entityVariables;
	currentGraph->assignTypes(nodeTypeCollection);
	currentEntity = curEntity;

	ConfigNode result = [&] () -> ConfigNode {
		const auto& node = graphState.getScriptGraphPtr()->getNodes().at(nodeId);
		const auto& nodeType = node.getNodeType();
		if (pinId == static_cast<GraphPinId>(-1)) {
			return nodeType.getDevConData(*this, node, graphState.getNodeState(nodeId).data);
		} else {
			const auto& pinConfig = nodeType.getPinConfiguration(node)[pinId];
			if (pinConfig.type == GraphElementType(ScriptNodeElementType::ReadDataPin)) {
				if (pinConfig.direction == GraphNodePinDirection::Input) {
					return readInputDataPin(node, pinId);
				} else {
					return readOutputDataPin(node, pinId);
				}
			} else if (pinConfig.type == GraphElementType(ScriptNodeElementType::TargetPin)) {
				EntityId id;
				if (pinConfig.direction == GraphNodePinDirection::Input) {
					if (node.getPins()[pinId].hasConnection()) {
						id = readInputEntityIdRaw(node, pinId);
					} else {
						id = readInputEntityId(node, pinId);
					}
				} else {
					id = readOutputEntityId(node, pinId);
				}
				const auto entityRef = world.tryGetEntity(id);
				if (id.isValid() && entityRef.isValid()) {
					return ConfigNode("Entity \"" + entityRef.getName() + "\" (id " + toString(id.value) + ")");
				} else {
					return ConfigNode(String("Invalid entity"));
				}
			} else {
				// No relevant data
				return {};
			}
		}
	}();

	currentGraph = nullptr;
	currentState = nullptr;
	currentEntityVariables = nullptr;
	currentEntity = EntityId();

	return result;
}

void ScriptEnvironment::doTerminateState()
{
	for (auto& thread: currentState->getThreads()) {
		terminateThread(thread, false);
	}
	currentState->getThreads().clear();

	for (auto& node: currentGraph->getNodes()) {
		if (node.getType() == "destructor") {
			runDestructor(node.getId());
		}
	}
}

void ScriptEnvironment::runDestructor(GraphNodeId nodeId)
{
	Vector<ScriptStateThread> threads;
	auto& t = threads.emplace_back(startThread(ScriptStateThread(nodeId, 0)));
	t.getTimeSlice() = std::numeric_limits<float>::infinity();

	while (true) {
		Vector<ScriptStateThread> pending;
		for (auto& t: threads) {
			updateThread(*currentState, t, pending);
		}
		if (pending.empty()) {
			break;
		} else {
			threads = std::move(pending);
		}
	}
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

void ScriptEnvironment::advanceThread(ScriptStateThread& thread, OptionalLite<GraphNodeId> node, GraphPinId outputPin, GraphPinId inputPin)
{
	if (node) {
		auto& state = currentState->getNodeState(node.value());
		if (state.threadCount == 0 && !thread.isWatcher()) {
			initNode(node.value(), state);
			thread.advanceToNode(node, outputPin, inputPin);
			return;
		}
	}

	terminateThread(thread, true);
}

void ScriptEnvironment::initNode(GraphNodeId nodeId, ScriptState::NodeState& nodeState)
{
	assert(nodeState.threadCount == 0);
	nodeState.threadCount++;
	currentState->startNode(currentGraph->getNodes()[nodeId], nodeState);
}

size_t ScriptEnvironment::forkThread(ScriptStateThread& thread, std::array<IScriptNodeType::OutputNode, 8> outputNodes, Vector<ScriptStateThread>& pendingThreads, size_t firstIdx)
{
	size_t n = 0;
	for (size_t j = firstIdx; j < outputNodes.size(); ++j) {
		if (outputNodes[j].dstNode && currentState->getNodeState(outputNodes[j].dstNode.value()).threadCount == 0) {
			addThread(thread.fork(outputNodes[j].dstNode.value(), outputNodes[j].outputPin, outputNodes[j].inputPin), pendingThreads);
			++n;
		}
	}
	return n;
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
	thread.advanceToNode({}, 0, 0);
	
	auto& state = *currentState;
	
	auto& threadStack = thread.getStack();
	const auto n = static_cast<int>(threadStack.size());
	for (int i = n; --i >= 0;) {
		const auto nodeId = threadStack[i].node;
		const auto& node = currentGraph->getNodes()[nodeId];

		auto& nodeState = state.getNodeState(nodeId);

		if (allowRollback && i >= 1 && node.getNodeType().isStackRollbackPoint(*this, node, threadStack[i].outputPin, nodeState.data)) {
			threadStack.resize(i);
			thread.advanceToNode(nodeId, threadStack[i - 1].outputPin, threadStack[i - 1].inputPin);
			return;
		}

		if (!thread.isWatcher()) {
			assert(nodeState.threadCount > 0);
			nodeState.threadCount--;
		}

		if (nodeState.threadCount == 0) {
			if (node.getNodeType().hasDestructor(node)) {
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

void ScriptEnvironment::setWatcher(ScriptStateThread& thread, bool newState)
{
	if (thread.isWatcher() != newState) {
		if (newState && (!thread.getCurNode() || currentState->getNodeState(*thread.getCurNode()).threadCount == 1)) {
			// If this is the last thread on this node, don't bother setting as watcher, terminate instead
			terminateThread(thread, false);
			return;
		}

		thread.setWatcher(newState);

		auto updateNode = [&](int nodeId)
		{
			auto& nThreads = currentState->getNodeState(nodeId).threadCount;
			if (newState) {
				assert(nThreads >= 2);
				--nThreads;
			} else {
				assert(nThreads >= 1);
				++nThreads;
			}
		};

		for (const auto s: thread.getStack()) {
			updateNode(s.node);
		}
		if (thread.getCurNode()) {
			updateNode(*thread.getCurNode());
		}
	}
}

void ScriptEnvironment::cancelOutputs(GraphNodeId nodeId, uint8_t cancelMask)
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

void ScriptEnvironment::abortCodePath(GraphNodeId node, std::optional<GraphPinId> outputPin)
{
	Expects(currentState != nullptr);

	for (auto& thread: currentState->getThreads()) {
		if (thread.stackGoesThrough(node, outputPin)) {
			terminateThread(thread, false);
		}
	}
}

void ScriptEnvironment::callFunction(ScriptStateThread& thread)
{
	const auto nodeId = thread.getCurNode().value();
	advanceThread(thread, currentGraph->getCallee(nodeId), 0, 0);
}

void ScriptEnvironment::returnFromFunction(ScriptStateThread& thread, uint8_t outputPins, Vector<ScriptStateThread>& pendingThreads)
{
	const auto returnNodeId = thread.getCurNode().value();
	const auto nodeId = currentGraph->getReturnTo(returnNodeId);

	if (nodeId) {
		const auto& node = currentGraph->getNodes()[*nodeId];
		const auto& nodeType = node.getNodeType();
		const auto outputNodes = nodeType.getOutputNodes(node, outputPins);

		forkThread(thread, outputNodes, pendingThreads, 1);
		advanceThread(thread, outputNodes[0].dstNode, outputNodes[0].outputPin, outputNodes[0].inputPin);
	} else {
		advanceThread(thread, {}, 0, 0);
	}
}

void ScriptEnvironment::processMessages(Time time, Vector<ScriptStateThread>& pending)
{
	Vector<GraphNodeId> toStart;
	currentState->processMessages(toStart);
	for (const auto nodeId: toStart) {
		pending.push_back(startThread(ScriptStateThread(nodeId, 0)));
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

EntityRef ScriptEnvironment::tryGetEntity(EntityId entityId) const
{
	return world.tryGetEntity(entityId.isValid() ? entityId : currentEntity);
}

const ScriptGraph* ScriptEnvironment::getCurrentGraph() const
{
	return currentGraph;
}

size_t& ScriptEnvironment::getNodeCounter(GraphNodeId nodeId)
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

bool ScriptEnvironment::isInputEnabled() const
{
	return inputEnabled;
}

void ScriptEnvironment::setInputDevice(EntityId target, std::shared_ptr<InputVirtual> input)
{
	inputDevices[target] = std::move(input);
}

std::shared_ptr<InputVirtual> ScriptEnvironment::getInputDevice(EntityId target, bool bypassEnableCheck) const
{
	if (!inputEnabled && !bypassEnableCheck) {
		return {};
	}

	const auto iter = inputDevices.find(target);
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

GraphPinId ScriptEnvironment::getCurrentInputPin() const
{
	return currentInputPin;
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

	const auto entity = tryGetEntity(dstEntity);
	if (entity.isValid()) {
		if (entity.hasComponent<ScriptableComponent>()) {
			if (dstEntity == currentEntity && message.type.script == currentState->getScriptId() && message.delay <= 0.00001f) {
				// Quick path for instant self messages
				currentState->receiveMessage(std::move(message));
			} else {
				scriptOutbox.emplace_back(dstEntity, std::move(message));
			}
		} else {
			Logger::logWarning("Trying to send message \"" + message.type.message + "\" to entity \"" + entity.getName() + "\", but it doesn't have a ScriptableComponent.");
		}
	}
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

void ScriptEnvironment::startScript(EntityId target, const String& scriptName, Vector<String> tags)
{
	scriptExecutionRequestOutbox.emplace_back(ScriptExecutionRequest{ ScriptExecutionRequestType::Start, target, scriptName, std::move(tags) });
}

void ScriptEnvironment::stopScript(EntityId target, const String& scriptName)
{
	scriptExecutionRequestOutbox.emplace_back(ScriptExecutionRequest{ ScriptExecutionRequestType::Stop, target, scriptName });
}

void ScriptEnvironment::stopScriptTag(EntityId target, const String& tag)
{
	scriptExecutionRequestOutbox.emplace_back(ScriptExecutionRequest{ ScriptExecutionRequestType::StopTag, target, tag });
}

Vector<std::pair<EntityId, ScriptMessage>> ScriptEnvironment::getOutboundScriptMessages()
{
	return std::move(scriptOutbox);
}

Vector<ScriptEnvironment::EntityMessageData> ScriptEnvironment::getOutboundEntityMessages()
{
	return std::move(entityOutbox);
}

Vector<ScriptEnvironment::ScriptExecutionRequest> ScriptEnvironment::getScriptExecutionRequests()
{
	return std::move(scriptExecutionRequestOutbox);
}

std::shared_ptr<UIWidget> ScriptEnvironment::createInWorldUI(const String& ui, Vector2f offset, Vector2f alignment, EntityId entityId)
{
	return {};
}

std::shared_ptr<UIWidget> ScriptEnvironment::createModalUI(const String& ui, ConfigNode data)
{
	return {};
}

EntityId ScriptEnvironment::getScriptTarget(const String& id) const
{
	if (scriptTargetRetriever) {
		return scriptTargetRetriever(id);
	}
	return {};
}

void ScriptEnvironment::setScriptTargetRetriever(ScriptTargetRetriever scriptTargetRetriever)
{
	this->scriptTargetRetriever = std::move(scriptTargetRetriever);
}

ScriptEnvironment::LockStatus ScriptEnvironment::getLockStatus(EntityId playerId, EntityId targetId) const
{
	// TODO
	return LockStatus::Unlocked;
}

bool ScriptEnvironment::isPendingLockResponse(int32_t token) const
{
	// TODO
	return false;
}

std::optional<int32_t> ScriptEnvironment::lockAcquire(EntityId playerId, EntityId targetId)
{
	// TODO
	return std::nullopt;
}

void ScriptEnvironment::lockRelease(EntityId playerId, EntityId targetId)
{
	// TODO
}

IScriptStateData* ScriptEnvironment::getNodeData(GraphNodeId nodeId)
{
	return currentState->getNodeState(nodeId).data;
}

void ScriptEnvironment::assignTypes(const ScriptGraph& graph)
{
	graph.assignTypes(nodeTypeCollection);
}

ConfigNode ScriptEnvironment::readInputDataPin(const ScriptGraphNode& node, GraphPinId pinN)
{
	const auto& pins = node.getPins();
	if (pinN >= pins.size()) {
		return {};
	}

	const auto& pin = pins[pinN];
	if (pin.connections.empty() || !pin.connections[0].dstNode) {
		return {};
	}
	assert(pin.connections.size() == 1);

	const auto& dst = pin.connections[0];
	const auto& nodes = currentGraph->getNodes();
	const auto& dstNode = nodes[dst.dstNode.value()];
	return dstNode.getNodeType().getData(*this, dstNode, dst.dstPin, getNodeData(dst.dstNode.value()));
}

ConfigNode ScriptEnvironment::readOutputDataPin(const ScriptGraphNode& node, GraphPinId pinN)
{
	return node.getNodeType().getData(*this, node, pinN, getNodeData(node.getId()));
}

EntityId ScriptEnvironment::readInputEntityIdRaw(const ScriptGraphNode& node, GraphPinId pinN)
{
	if (pinN < node.getPins().size()) {
		const auto& pin = node.getPins()[pinN];
		if (!pin.connections.empty()) {
			const auto& conn = pin.connections[0];
			if (conn.dstNode) {
				const auto& nodes = getCurrentGraph()->getNodes();
				const auto& dstNode = nodes.at(conn.dstNode.value());
				return dstNode.getNodeType().getEntityId(*this, dstNode, conn.dstPin, getNodeData(conn.dstNode.value()));
			}
		}
	}
	return EntityId();
}

EntityId ScriptEnvironment::readInputEntityId(const ScriptGraphNode& node, GraphPinId pinN)
{
	const auto entityId = readInputEntityIdRaw(node, pinN);
	return entityId.isValid() ? entityId : currentEntity;
}

EntityId ScriptEnvironment::readOutputEntityId(const ScriptGraphNode& node, GraphPinId pinN)
{
	return node.getNodeType().getEntityId(*this, node, pinN, getNodeData(node.getId()));
}

void ScriptEnvironment::postAudioEvent(const String& id, EntityId entityId)
{
	if (!id.isEmpty()) {
		if (const auto* audioSource = tryGetComponent<AudioSourceComponent>(entityId)) {
			api.audio->postEvent(id, audioSource->emitter);
		} else {
			api.audio->postEvent(id);
		}
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

const ScriptVariables& ScriptEnvironment::getEntityVariables(EntityId entityId) const
{
	auto entity = tryGetEntity(entityId);
	if (entity.isValid()) {
		auto* scriptable = entity.tryGetComponent<ScriptableComponent>();
		if (scriptable) {
			return scriptable->variables;
		}
	}

	static ScriptVariables dummy;
	return dummy;
}
