#include "halley/scripting/script_environment.h"
#include "halley/scripting/script_state_set.h"


#include "halley/entity/world.h"
#include "halley/api/halley_api.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "halley/scripting/script_graph.h"
#include "halley/scripting/script_state.h"
#include "halley/api/audio_api.h"
#include "halley/audio/audio_event.h"

#include "halley/audio/audio_position.h"
#include "halley/graphics/sprite/animation_player.h"
#include "halley/bytes/byte_serializer.h"
#include <components/audio_source_component.h>
#include <components/scriptable_component.h>
#include <components/sprite_animation_component.h>

#include "halley/entity/components/transform_2d_component.h"
#include "halley/support/profiler.h"
#include "halley/utils/scoped_guard.h"
#include "nodes/script_network.h"
#include "system_messages/play_network_animation_system_message.h"
#include "system_messages/play_network_sound_system_message.h"

using namespace Halley;

ScriptEnvironment::ScriptEnvironment(const HalleyAPI& api, World& world, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> nodeTypeCollection, bool isHost)
	: api(api)
	, world(world)
	, resources(resources)
	, nodeTypeCollection(std::move(nodeTypeCollection))
	, isHost(isHost)
{
	serializationContext.entityContext = this;
}

bool ScriptEnvironment::hasCurrentState() const
{
	return !stateStack.empty();
}

void ScriptEnvironment::pushState(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables, Time deltaTime)
{
	if (!graphState.getScriptGraphPtr()) {
		throw Exception("Unable to update script state, script not set.", HalleyExceptions::Entity);
	}
	if (!curEntity.isValid()) {
		throw Exception("Unable to update script state, invalid entityId.", HalleyExceptions::Entity);
	}

	auto& s = stateStack.emplace_back();
	s.deltaTime = deltaTime;
	s.graph = graphState.getScriptGraphPtr();
	s.state = &graphState;
	s.entityVariables = &entityVariables;
	s.graph->assignTypes(*nodeTypeCollection);
	s.entity = curEntity;
}

void ScriptEnvironment::pushStateCopy(const ScriptGraph& graph)
{
	auto& s = stateStack.emplace_back(getState());
	s.graph = &graph;
}

void ScriptEnvironment::popState()
{
	HalleyAssertDev(!stateStack.empty());
	stateStack.pop_back();
}

ScriptEnvironment::CurState& ScriptEnvironment::getState()
{
	HalleyAssertDev(!stateStack.empty());
	return stateStack.back();
}

const ScriptEnvironment::CurState& ScriptEnvironment::getState() const
{
	HalleyAssertDev(!stateStack.empty());
	return stateStack.back();
}

void ScriptEnvironment::updateState(Time time, ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables)
{
	pushState(graphState, curEntity, entityVariables, time);
	auto statePop = ScopedGuard([this] { popState(); });

	auto& s = getState();
	const auto trace1 = StackDebugTrace("scriptId", s.graph->getAssetId());
	const auto trace2 = StackDebugTrace("currentState", s.state);
	ProfilerEvent event(ProfilerEventType::ScriptUpdate, s.graph->getAssetId(), reinterpret_cast<uint64_t>(this));

	try {
		auto& threads = graphState.getThreads();

		const bool hashChanged = graphState.getGraphHash() != s.graph->getHash();
		if (!graphState.hasStarted() || hashChanged) {
			if (graphState.hasStarted()) {
				// i.e. we're here because the script changed
				terminateStateWith(s.graph->getPreviousVersion(graphState.getGraphHash()));
			}

			graphState.start(s.graph->getHash());
			graphState.prepareStates(serializationContext, time);
			if (s.graph->getStartNode()) {
				threads.push_back(startThread(ScriptStateThread(*s.graph->getStartNode(), 0)));
			}
		} else {
			graphState.prepareStates(serializationContext, time);
		}
		
		processMessages(time, threads);
		processControlEvents(time, threads);

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
		s.thread = nullptr;
		removeStoppedThreads();

		// Clean up if done
		if (graphState.isDone()) {
			doTerminateState();
		}

		graphState.updateDisplayOffset(time);
		graphState.incrementFrameNumber();
	} catch (const std::exception& e) {
		auto entity = getWorld().tryGetEntity(curEntity);
		String name = entity.isValid() ? entity.getName() : "<invalidEntity>";
		Logger::logError("Exception while executing script \"" + s.graph->getAssetId() + "\" attached to entity \"" + name + "\":");
		Logger::logException(e);
	}
}

bool ScriptEnvironment::updateThread(ScriptState& graphState, ScriptStateThread& thread, Vector<ScriptStateThread>& pendingThreads)
{
	auto& s = getState();
	s.thread = &thread;
	float& timeLeft = thread.getTimeSlice();

	std::array<IScriptNodeType::OutputNode, 32> outputBuffer;

	while (timeLeft > 0 && thread.isRunning()) {
		// Get node type
		const auto nodeId = thread.getCurNode().value();
		const auto& node = s.graph->getNodes().at(nodeId);
		const auto& nodeType = node.getNodeType();
		auto& nodeState = graphState.getNodeState(nodeId);
		s.inputPin = thread.getCurInputPin();

		const auto trace = StackDebugTrace("nodeType", node.getType());

		// Dead watcher
		if (nodeState.threadCount == nodeState.watcherCount && thread.isWatcher()) {
			terminateThread(thread, false);
			continue;
		}
		
		// Update
		const auto result = nodeType.update(*this, static_cast<Time>(timeLeft), node, nodeState.data);
		thread.getCurNodeTime() += timeLeft;
		timeLeft -= clamp(static_cast<float>(result.timeElapsed), 0.0f, timeLeft);
		HalleyAssertDev(result.timeElapsed >= 0);

		if (result.outputsCancelled != 0) {
			cancelOutputs(nodeId, result.outputsCancelled);
		}

		if (result.state == ScriptNodeExecutionState::Executing) {
			// Still running this node, suspend
			timeLeft = 0;
		} else if (result.state == ScriptNodeExecutionState::Fork || result.state == ScriptNodeExecutionState::ForkAndConvertToWatcher) {
			forkThread(thread, nodeType.getOutputNodes(node, result.outputsActive, outputBuffer), pendingThreads);
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

				const auto outputNodes = nodeType.getOutputNodes(node, result.outputsActive, outputBuffer);
				forkThread(thread, outputNodes, pendingThreads, 1);
				advanceThread(thread, outputNodes[0].dstNode, outputNodes[0].outputPin, outputNodes[0].inputPin);
			} else if (result.state == ScriptNodeExecutionState::Detach) {
				const auto outputNodes = nodeType.getOutputNodes(node, result.outputsActive, outputBuffer);
				advanceThread(thread, {}, 0, 0);
				forkThread(thread, outputNodes, pendingThreads, 0);
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
	}
	return true;
}

void ScriptEnvironment::terminateStateWith(const ScriptGraph* scriptGraph)
{
	if (scriptGraph) {
		pushStateCopy(*scriptGraph);
		auto statePop = ScopedGuard([this] { popState(); });
		doTerminateState();
		Logger::logDev("Script restarted after changing");
	} else {
		Logger::logError("Could not terminate state properly, previous state is missing?");
	}
}

void ScriptEnvironment::stopState(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables, bool allThreads)
{
	pushState(graphState, curEntity, entityVariables, 0);
	auto statePop = ScopedGuard([this] { popState(); });

	if (allThreads) {
		doTerminateState();
	} else {
		if (const auto startNodeId = getState().graph->getStartNode()) {
			abortCodePath(*startNodeId, {}, true);
		}
		const auto& threads = getState().state->getThreads();
		if (std::none_of(threads.begin(), threads.end(), [] (const ScriptStateThread& thread) { return thread.isRunning(); })) {
			doTerminateState();
		}
	}
}

void ScriptEnvironment::terminateState(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables)
{
	stopState(graphState, curEntity, entityVariables, true);
}

void ScriptEnvironment::doTerminateState()
{
	auto curState = getState().state;

	for (auto& thread: curState->getThreads()) {
		terminateThread(thread, false);
	}
	curState->getThreads().clear();

	if (!curState->isTerminated()) {
		for (auto& node: getState().graph->getNodes()) {
			if (node.getType() == "destructor") {
				runDestructor(node.getId());
			}
		}
	}

	curState->markTerminated();
}

void ScriptEnvironment::runDestructor(GraphNodeId nodeId)
{
	Vector<ScriptStateThread> threads;
	auto& t = threads.emplace_back(startThread(ScriptStateThread(nodeId, 0)));
	t.getTimeSlice() = std::numeric_limits<float>::max();

	const auto& s = getState();
	while (true) {
		Vector<ScriptStateThread> pending;
		for (auto& t: threads) {
			updateThread(*s.state, t, pending);
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
	const auto curState = getState().state;
	if (!curState->hasStarted()) {
		curState->start(getState().graph->getHash());
		curState->prepareStates(serializationContext, 0);
	}

	for (const auto& s: thread.getStack()) {
		auto& nThreads = curState->getNodeState(s.node).threadCount;
		HalleyAssertDev(nThreads >= 1);
		++nThreads;
	}

	if (thread.getCurNode()) {
		const auto nodeId = thread.getCurNode().value();
		initNode(nodeId, curState->getNodeState(nodeId));
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
		const auto trace = StackDebugTrace("currentState", getState().state);
		auto& state = getState().state->getNodeState(node.value());
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
	HalleyAssertDev(nodeState.threadCount == 0);
	nodeState.threadCount++;
	getState().state->startNode(getState().graph->getNodes()[nodeId], nodeState);
}

size_t ScriptEnvironment::forkThread(ScriptStateThread& thread, gsl::span<IScriptNodeType::OutputNode> outputNodes, Vector<ScriptStateThread>& pendingThreads, size_t firstIdx)
{
	size_t n = 0;
	for (size_t j = firstIdx; j < outputNodes.size(); ++j) {
		if (outputNodes[j].dstNode && getState().state->getNodeState(outputNodes[j].dstNode.value()).threadCount == 0) {
			addThread(thread.fork(outputNodes[j].dstNode.value(), outputNodes[j].outputPin, outputNodes[j].inputPin), pendingThreads);
			++n;
		}
	}
	return n;
}

void ScriptEnvironment::mergeThread(ScriptStateThread& thread, bool wait)
{
	for (auto& other: getState().state->getThreads()) {
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
	
	auto& state = *getState().state;
	
	auto& threadStack = thread.getStack();
	const auto n = static_cast<int>(threadStack.size());
	for (int i = n; --i >= 0;) {
		const auto nodeId = threadStack[i].node;
		const auto& node = getState().graph->getNodes()[nodeId];

		auto& nodeState = state.getNodeState(nodeId);

		if (allowRollback && i >= 1 && node.getNodeType().isStackRollbackPoint(*this, node, threadStack[i].outputPin, nodeState.data)) {
			if (nodeState.threadCount == 1) {
				threadStack.resize(i);
				thread.advanceToNode(nodeId, threadStack[i - 1].outputPin, threadStack[i - 1].inputPin);
				return;
			}
		}
		
		HalleyAssertDev(nodeState.threadCount > 0);
		nodeState.threadCount--;

		if (thread.isWatcher()) {
			HalleyAssertDev(nodeState.watcherCount > 0);
			nodeState.watcherCount--;
		}

		if (nodeState.threadCount == 0) {
			if (node.getNodeType().hasDestructor(node, *getState().graph)) {
				node.getNodeType().destructor(*this, node, nodeState.data);
			}
			state.finishNode(node, nodeState, true);
		}
	}
	threadStack.clear();
}

void ScriptEnvironment::removeStoppedThreads()
{
	std_ex::erase_if(getState().state->getThreads(), [&] (const ScriptStateThread& thread) { return !thread.getCurNode(); });
}

void ScriptEnvironment::setWatcher(ScriptStateThread& thread, bool newState)
{
	auto currentState = getState().state;
	if (thread.isWatcher() != newState) {
		if (newState && (!thread.getCurNode() || currentState->getNodeState(*thread.getCurNode()).threadCount == currentState->getNodeState(*thread.getCurNode()).watcherCount + 1)) {
			// If this is the last non-watcher thread on this node, don't bother setting as watcher, terminate instead
			terminateThread(thread, false);
			return;
		}

		thread.setWatcher(newState);

		auto updateNode = [&](int nodeId)
		{
			const auto& nThreads = currentState->getNodeState(nodeId).threadCount;
			auto& nWatchers = currentState->getNodeState(nodeId).watcherCount;
			if (newState) {
				++nWatchers;
			} else {
				HalleyAssertDev(nWatchers >= 1);
				--nWatchers;
			}
			HalleyAssertDev(nWatchers <= nThreads);
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
		abortCodePath(nodeId, {}, false);
	} else {
		for (uint8_t i = 0; i < 8; ++i) {
			if ((cancelMask & (1 << i)) != 0) {
				auto& node = getState().graph->getNodes()[nodeId];
				const auto pinIdx = node.getNodeType().getNthOutputPinIdx(node, i);
				HalleyAssertDev(node.getPinType(pinIdx).isCancellable);
				abortCodePath(nodeId, pinIdx, false);
			}
		}
	}
}

void ScriptEnvironment::abortCodePath(GraphNodeId node, std::optional<GraphPinId> outputPin, bool includeCurNode)
{
	for (auto& thread: getState().state->getThreads()) {
		if (thread.stackGoesThrough(node, outputPin) || (includeCurNode && thread.getCurNode() == node)) {
			terminateThread(thread, false);
		}
	}
}

void ScriptEnvironment::callFunction(ScriptStateThread& thread)
{
	const auto nodeId = thread.getCurNode().value();
	advanceThread(thread, getState().graph->getCallee(nodeId), 0, 0);
}

void ScriptEnvironment::returnFromFunction(ScriptStateThread& thread, uint8_t outputPins, Vector<ScriptStateThread>& pendingThreads)
{
	const auto returnNodeId = thread.getCurNode().value();
	const auto nodeId = getState().graph->getReturnTo(returnNodeId);

	if (nodeId) {
		const auto& node = getState().graph->getNodes()[*nodeId];
		const auto& nodeType = node.getNodeType();
		std::array<IScriptNodeType::OutputNode, 32> outputBuffer;
		const auto outputNodes = nodeType.getOutputNodes(node, outputPins, outputBuffer);

		forkThread(thread, outputNodes, pendingThreads, 1);
		advanceThread(thread, outputNodes[0].dstNode, outputNodes[0].outputPin, outputNodes[0].inputPin);
	} else {
		advanceThread(thread, {}, 0, 0);
	}
}

void ScriptEnvironment::processMessages(Time time, Vector<ScriptStateThread>& pending)
{
	Vector<GraphNodeId> toStart;
	getState().state->processMessages(toStart);
	for (const auto nodeId: toStart) {
		pending.push_back(startThread(ScriptStateThread(nodeId, 0)));
	}
}

void ScriptEnvironment::processControlEvents(Time time, Vector<ScriptStateThread>& pending)
{
	std::array<IScriptNodeType::OutputNode, 32> outputBuffer;
	const auto& s = getState();
	for (auto& event: s.state->processControlEvents()) {
		if (event.type == ScriptState::ControlEventType::StartThread) {
			const auto& node = s.graph->getNodes()[event.nodeId];
			const auto& nodeType = node.getNodeType();

			const auto outputs = nodeType.getOutputNodes(node, 1, outputBuffer);
			if (const auto dstNode = outputs[0].dstNode) {
				const auto dstPin = outputs[0].inputPin;
				pending.push_back(startThread(ScriptStateThread(*dstNode, dstPin)));
				pending.back().setNetworkOwnerId(event.networkOwnerId);
			}

			auto* nodeData = dynamic_cast<ScriptTransferToHostData*>(getNodeData(event.nodeId));
			dynamic_cast<const ScriptTransferToHost&>(nodeType).setParameters(node, *nodeData, std::move(event.params));
		} else if (event.type == ScriptState::ControlEventType::CancelThread) {
			const auto& node = s.graph->getNodes()[event.nodeId];
			const auto& nodeType = node.getNodeType();
			const auto outputs = nodeType.getOutputNodes(node, 1, outputBuffer);
			if (const auto dstNode = outputs[0].dstNode) {
				abortCodePath(*dstNode, {}, true);
			}
		} else if (event.type == ScriptState::ControlEventType::NotifyReturn) {
			const auto& node = s.graph->getNodes()[event.nodeId];
			const auto& nodeType = node.getNodeType();
			auto* nodeData = dynamic_cast<ScriptTransferToHostData*>(getNodeData(event.nodeId));
			dynamic_cast<const ScriptTransferToHost&>(nodeType).notifyReturn(node, *nodeData, std::move(event.params));
		}
	}
}

EntityId ScriptEnvironment::getEntityIdFromUUID(const UUID& uuid) const
{
	auto e = world.findEntity(uuid);
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

World* ScriptEnvironment::getEntityFactoryContextWorld() const
{
	return &world;
}

EntityRef ScriptEnvironment::tryGetEntity(EntityId entityId) const
{
	return world.tryGetEntity(entityId.isValid() ? entityId : getState().entity);
}

const ScriptGraph* ScriptEnvironment::getCurrentGraph() const
{
	return getState().graph;
}

size_t& ScriptEnvironment::getNodeCounter(GraphNodeId nodeId)
{
	return getState().state->getNodeCounter(nodeId);
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

bool ScriptEnvironment::hasInputLabel(EntityId entityId) const
{
	for (auto& input: inputDevices) {
		if (input.second) {
			for (const auto& button: input.second->getExclusiveButtonLabels(nullptr)) {
				if (button.label.target == entityId) {
					return true;
				}
			}
		}
	}
	return false;
}

void ScriptEnvironment::setHostNetworkAuthority(bool host)
{
	isHost = host;
}

bool ScriptEnvironment::hasNetworkAuthorityOver(EntityId id)
{
	return hasNetworkAuthorityOver(tryGetEntity(id));
}

bool ScriptEnvironment::hasNetworkAuthorityOver(EntityRef entity) const
{
	return world.isEntityNetworkAuthority(entity);
}

bool ScriptEnvironment::hasHostNetworkAuthority() const
{
	return isHost;
}

bool ScriptEnvironment::isNetworkConnected() const
{
	return world.isNetworkConnected();
}

int ScriptEnvironment::getCurrentFrameNumber() const
{
	return getState().state->getCurrentFrameNumber();
}

Time ScriptEnvironment::getDeltaTime() const
{
	return getState().deltaTime;
}

EntityId ScriptEnvironment::getCurrentEntityId() const
{
	return hasCurrentState() ? getState().entity : EntityId();
}

GraphPinId ScriptEnvironment::getCurrentInputPin() const
{
	return getState().inputPin;
}

World& ScriptEnvironment::getWorld()
{
	return world;
}

Resources& ScriptEnvironment::getResources()
{
	return resources;
}

void ScriptEnvironment::sendScriptMessage(EntityId dstEntity, ScriptMessage message, std::optional<SystemMessageDestination> destination)
{
	const auto curEntity = getCurrentEntityId();
	auto* curState = hasCurrentState() ? getState().state : nullptr;

	if (!dstEntity.isValid()) {
		dstEntity = curEntity;
	}

	const auto entity = tryGetEntity(dstEntity);
	if (!entity.isValid()) {
		Logger::logError("Unable to send script message: could not find entity " + toString(dstEntity));
		return;
	}

	if ((!destination && entity.isLocal()) || destination == SystemMessageDestination::Local) {
		// Send local message

		if (!entity.hasComponent<ScriptableComponent>()) {
			Logger::logError("Trying to send message \"" + message.type.message + "\" to entity \"" + entity.getName() + "\", but it doesn't have a ScriptableComponent.");
			return;
		}

		if (dstEntity == curEntity && curState && message.type.script == curState->getScriptId() && message.delay <= 0.00001f) {
			// Quick path for instant self messages
			curState->receiveMessage(std::move(message));
		} else {
			scriptOutbox.emplace_back(dstEntity, std::move(message));
		}
	} else {
		// Send remote message
		getInterface<IScriptSystemInterface>().sendScriptMessage(dstEntity, std::move(message), destination);
	}
}

void ScriptEnvironment::sendEntityMessage(EntityMessageData message)
{
	HalleyAssertDev(!message.messageName.isEmpty());

	if (!message.targetEntity.isValid()) {
		message.targetEntity = getCurrentEntityId();
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
	context.callback = std::move(message.callback);

	world.sendSystemMessage(std::move(context), message.targetSystem, dst);
}

void ScriptEnvironment::startScript(EntityId target, const String& scriptName, Vector<String> tags, Vector<ConfigNode> params)
{
	scriptExecutionRequestOutbox.emplace_back(ScriptExecutionRequest{ ScriptExecutionRequestType::Start, target, scriptName, std::move(tags), std::move(params), false, true });
}

void ScriptEnvironment::stopScript(EntityId target, const String& scriptName, bool allThreads, bool matching)
{
	scriptExecutionRequestOutbox.emplace_back(ScriptExecutionRequest{ ScriptExecutionRequestType::Stop, target, scriptName, {}, {}, allThreads, matching });
}

void ScriptEnvironment::stopScriptTag(EntityId target, const String& tag, bool allThreads, bool matching)
{
	scriptExecutionRequestOutbox.emplace_back(ScriptExecutionRequest{ ScriptExecutionRequestType::StopTag, target, tag, {}, {}, allThreads, matching });
}

Vector<ScriptEnvironment::ScriptExecutionRequest> ScriptEnvironment::getScriptExecutionRequests()
{
	return std::move(scriptExecutionRequestOutbox);
}

bool ScriptEnvironment::hasStopRequests() const
{
	for (auto& request: scriptExecutionRequestOutbox) {
		if (request.type == ScriptExecutionRequestType::Stop || request.type == ScriptExecutionRequestType::StopTag) {
			return true;
		}
	}
	return false;
}

Vector<std::pair<EntityId, ScriptMessage>> ScriptEnvironment::getOutboundScriptMessages()
{
	return std::move(scriptOutbox);
}

Vector<ScriptEnvironment::EntityMessageData> ScriptEnvironment::getOutboundEntityMessages()
{
	return std::move(entityOutbox);
}

void ScriptEnvironment::startHostThread(int node, ConfigNode params)
{
	getInterface<IScriptSystemInterface>().startHostThread(getState().entity, getState().graph->getAssetId(), node, std::move(params));
}

void ScriptEnvironment::cancelHostThread(int node)
{
	getInterface<IScriptSystemInterface>().cancelHostThread(getState().entity, getState().graph->getAssetId(), node);
}

void ScriptEnvironment::returnHostThread(ConfigNode params)
{
	const auto& s = getState();
	const auto threadRootId = s.thread->getStack()[0].node;

	OptionalLite<GraphNodeId> rootNodeId;

	const auto& node = s.graph->getNodes()[threadRootId];
	const auto& pinConfigs = node.getNodeType().getPinConfiguration(node);
	const auto& pins = node.getPins();
	for (size_t i = 0; i < pins.size(); ++i) {
		if (pinConfigs[i].type == GraphElementType(ScriptNodeElementType::FlowPin) && pinConfigs[i].direction == GraphNodePinDirection::Input) {
			for (const auto& conn: pins[i].connections) {
				if (conn.dstNode) {
					rootNodeId = conn.dstNode;
					break;
				}
			}			
		}
	}

	if (rootNodeId) {
		getInterface<IScriptSystemInterface>().sendReturnHostThread(s.entity, s.graph->getAssetId(), *rootNodeId, std::move(params));
	}
}

std::shared_ptr<UIWidget> ScriptEnvironment::createInWorldUI(const String& ui, Vector2f offset, Vector2f alignment, EntityId entityId, ConfigNode data)
{
	return {};
}

std::shared_ptr<UIWidget> ScriptEnvironment::createModalUI(const String& ui, ConfigNode data)
{
	return {};
}

EntityId ScriptEnvironment::getScriptTarget(const String& id, bool warnIfMissing) const
{
	if (scriptTargetRetriever) {
		const EntityId target = scriptTargetRetriever(id);
		if (warnIfMissing && !target.isValid()) {
			Logger::logError("Unable to find entity with ScriptTarget id \"" + id + "\"", true);
		}
		return target;
	} else {
		Logger::logError("Unable to get script target: scriptTargetRetriever is not set.", true);
	}
	return {};
}

void ScriptEnvironment::setScriptTargetRetriever(ScriptTargetRetriever scriptTargetRetriever)
{
	this->scriptTargetRetriever = std::move(scriptTargetRetriever);
}

gsl::span<const ConfigNode> ScriptEnvironment::getStartParams() const
{
	return getState().state ? getState().state->getStartParams() : gsl::span<const ConfigNode>();
}

const ScriptNodeTypeCollection& ScriptEnvironment::getNodeTypeCollection() const
{
	return *nodeTypeCollection;
}

void ScriptEnvironment::setFutureNodeValue(const ScriptGraphNode& node, std::optional<Future<ConfigNode>> future)
{
	if (getState().state) {
		getState().state->setFutureNodeValue(node.getId(), std::move(future));
	}
}

std::optional<Future<ConfigNode>> ScriptEnvironment::getFutureNodeValue(const ScriptGraphNode& node)
{
	if (getState().state) {
		return getState().state->getFutureNodeValue(node.getId());
	}
	return {};
}

bool ScriptEnvironment::isDevMode() const
{
	return api.core->isDevMode();
}

IScriptStateData* ScriptEnvironment::getNodeData(GraphNodeId nodeId)
{
	return getState().state->getNodeState(nodeId).data;
}

void ScriptEnvironment::assignTypes(const ScriptGraph& graph)
{
	graph.assignTypes(*nodeTypeCollection);
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
	HalleyAssertDev(pin.connections.size() == 1);

	const auto& dst = pin.connections[0];
	const auto& dstNode = getState().graph->getNodes()[dst.dstNode.value()];
	return dstNode.getNodeType().getData(*this, dstNode, dst.dstPin, getNodeData(dst.dstNode.value()));
}

ConfigNode ScriptEnvironment::readOutputDataPin(const ScriptGraphNode& node, GraphPinId pinN)
{
	return node.getNodeType().getData(*this, node, pinN, getNodeData(node.getId()));
}

EntityId ScriptEnvironment::readInputEntityId(const ScriptGraphNode& node, GraphPinId pinN, bool disconnectedIsSelf)
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
	return disconnectedIsSelf ? getState().entity : EntityId();
}

EntityId ScriptEnvironment::readInputEntityIdRaw(const ScriptGraphNode& node, GraphPinId pinN)
{
	return readInputEntityId(node, pinN, false);
}

EntityId ScriptEnvironment::readInputEntityId(const ScriptGraphNode& node, GraphPinId pinN)
{
	return readInputEntityId(node, pinN, true);
}

EntityId ScriptEnvironment::readOutputEntityId(const ScriptGraphNode& node, GraphPinId pinN)
{
	return node.getNodeType().getEntityId(*this, node, pinN, getNodeData(node.getId()));
}

void ScriptEnvironment::postAnimationEvent(const String& sequence, bool reverse, bool once, EntityId entityId)
{
	if (!sequence.isEmpty()) {
		auto msg = std::make_unique<PlayNetworkAnimationSystemMessage>(entityId, std::nullopt, sequence, reverse, once, "default");
		const auto msgId = msg->getId();

		SystemMessageContext context;
		context.msg = std::move(msg);
		context.msgId = msgId;
		context.remote = false;
		context.callback = nullptr;

		world.sendSystemMessage(std::move(context), "SpriteAnimation", SystemMessageDestination::RemoteClients);
	}
}

void ScriptEnvironment::postAudioEvent(const String& id, EntityId entityId)
{
	if (!id.isEmpty()) {
		if (resources.exists<AudioEvent>(id)) {
			// Check scope of actions stored in this audio event:
			// If there are any actions that are not object-scoped, don't broadcast as a networked system message.
			//
			// NB: This is ambiguous - if an audio event uses actions in both global and object scope, it won't broadcast either.
			const auto& actions = resources.get<AudioEvent>(id)->getActions();

			size_t numObjectScoped = 0;
			for (const auto& action : actions) {
				if (action->getScope() == AudioEventScope::Object) {
					numObjectScoped++;
				}
			}

			if (numObjectScoped < actions.size()) {
				getInterface<IAudioSystemInterface>().playAudio(id, entityId);
				return;
			}
		}

		auto msg = std::make_unique<PlayNetworkSoundSystemMessage>(entityId, id);
		const auto msgId = msg->getId();

		SystemMessageContext context;
		context.msg = std::move(msg);
		context.msgId = msgId;
		context.remote = false;
		context.callback = nullptr;

		world.sendSystemMessage(std::move(context), "Audio", SystemMessageDestination::AllClients);
	}
}

ScriptVariables& ScriptEnvironment::getVariables(ScriptVariableScope scope)
{
	switch (scope) {
	case ScriptVariableScope::Local:
		return getState().state->getLocalVariables();
	case ScriptVariableScope::Shared:
		return getState().state->getSharedVariables();
	case ScriptVariableScope::Entity:
		return *getState().entityVariables;
	default:
		throw Exception("Variable type " + toString(scope) + " not implemented", HalleyExceptions::Entity);
	}
}

const ScriptVariables& ScriptEnvironment::getVariables(ScriptVariableScope scope) const
{
	switch (scope) {
	case ScriptVariableScope::Local:
		return getState().state->getLocalVariables();
	case ScriptVariableScope::Shared:
		return getState().state->getSharedVariables();
	case ScriptVariableScope::Entity:
		return *getState().entityVariables;
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

void ScriptEnvironment::setEntityVariable(EntityId entityId, const String& name, ConfigNode value) const
{
	auto entity = tryGetEntity(entityId);
	if (entity.isValid()) {
		auto* scriptable = entity.tryGetComponent<ScriptableComponent>();
		if (scriptable) {
			scriptable->variables.setVariable(name, std::move(value));
		}
	}
}

void ScriptEnvironment::setVariableTable(const VariableTable& variableTable)
{
	this->variableTable = &variableTable;
}

const VariableTable* ScriptEnvironment::getVariableTable() const
{
	return variableTable;
}

ConfigNode ScriptEnvironment::readNodeElementDevConData(ScriptState& graphState, EntityId curEntity, ScriptVariables& entityVariables, GraphNodeId nodeId, GraphPinId pinId)
{
	pushState(graphState, curEntity, entityVariables, 0);
	auto statePop = ScopedGuard([this] { popState(); });

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
				if (nodeType.isPinConnected(node, pinId)) {
					if (pinConfig.direction == GraphNodePinDirection::Input) {
						id = readInputEntityId(node, pinId);
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
					return ConfigNode(String("<empty>"));
				}
			} else {
				// No relevant data
				return {};
			}
		}
	}();

	return result;
}
