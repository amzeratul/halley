#include <systems/script_system.h>

using namespace Halley;

class ScriptSystem final : public ScriptSystemBase<ScriptSystem>, IScriptSystemInterface, IEntityFactoryContext {
public:
	void init()
	{
		getWorld().setInterface<IScriptSystemInterface>(this);

		addConsoleCommands();
	}

	void update(Time t)
	{
		initializeEnvironment();
		initializeScripts();
		bool run = true;
		while (run) {
			updateScripts(t);
			updatePendingMessages(t);
			run = fulfillScriptExecutionRequests();
			sendMessages();
		}

		if (getDevService().isDevMode()) {
			updateDevCon();
		}
	}

	void onEntitiesRemoved(Span<ScriptableFamily> es)
	{
		auto& env = getScriptingService().getEnvironment();
		for (auto& e: es) {
			for (auto& state: e.scriptable.activeStates) {
				if (state->getScriptGraphPtr()->isPersistent()) {
					// TODO?
				} else {
					env.terminateState(*state, e.entityId, e.scriptable.variables);
				}
			}
			e.scriptable.activeStates.clear();
		}
	}

	void onMessageReceived(const StartScriptMessage& msg, ScriptableFamily& e) override
	{
		if (getResources().exists<ScriptGraph>(msg.name)) {
			addScript(e.entityId, e.scriptable, getResources().get<ScriptGraph>(msg.name), msg.tags, msg.params);
		} else {
			Logger::logError("Script not found: \"" + msg.name + "\"");
		}
	}

	void onMessageReceived(StartHostScriptThreadSystemMessage msg) override
	{
		if (const auto scriptable = getScriptState(msg.entity, msg.script)) {
			scriptable->receiveControlEvent(ScriptState::ControlEvent{ ScriptState::ControlEventType::StartThread, static_cast<GraphNodeId>(msg.nodeId), ConfigNode(msg.params), msg.fromPeerId });
		} else {
			Logger::logWarning("Couldn't find script " + msg.script + " on entity " + toString(msg.entity));
		}
	}

	void onMessageReceived(CancelHostScriptThreadSystemMessage msg) override
	{
		if (const auto scriptable = getScriptState(msg.entity, msg.script)) {
			scriptable->receiveControlEvent(ScriptState::ControlEvent{ ScriptState::ControlEventType::CancelThread, static_cast<GraphNodeId>(msg.nodeId), ConfigNode(), msg.fromPeerId });
		} else {
			Logger::logWarning("Couldn't find script " + msg.script + " on entity " + toString(msg.entity));
		}
	}

	void onMessageReceived(const ReturnHostScriptThreadMessage& msg, ScriptableFamily& e) override
	{
		if (const auto scriptable = getScriptState(e, msg.script)) {
			scriptable->receiveControlEvent(ScriptState::ControlEvent{ ScriptState::ControlEventType::NotifyReturn, static_cast<GraphNodeId>(msg.nodeId), ConfigNode(msg.params), msg.fromPeerId });
		} else {
			Logger::logWarning("Couldn't find script " + msg.script + " on entity " + toString(e.entityId));
		}
	}
	
	std::shared_ptr<ScriptState> addScript(EntityId entityId, const String& scriptName, Vector<String> tags = {}, Vector<ConfigNode> params = {}) override
	{
		if (auto* scriptable = scriptableFamily.tryFind(entityId)) {
			if (getResources().exists<ScriptGraph>(scriptName)) {
				return addScript(entityId, scriptable->scriptable, getResources().get<ScriptGraph>(scriptName), std::move(tags), std::move(params));
			} else {
				Logger::logError("Script not found: " + scriptName);
				return {};
			}
		} else {
			return {};
		}
	}

	void onMessageReceived(const TerminateScriptMessage& msg, ScriptableFamily& e) override
	{
		auto& env = getScriptingService().getEnvironment();
		for (auto& script: e.scriptable.activeStates) {
			if (script->getScriptId() == msg.name) {
				env.terminateState(*script, e.entityId, e.scriptable.variables);
			}
		}
		e.scriptable.activeStates.removeDeadStates();
	}

	void onMessageReceived(const TerminateScriptsWithTagMessage& msg, ScriptableFamily& e) override
	{
		auto& env = getScriptingService().getEnvironment();
		for (auto& state: e.scriptable.activeStates) {
			if (state->hasTag(msg.tag)) {
				env.terminateState(*state, e.entityId, e.scriptable.variables);
			}
		}
		e.scriptable.activeStates.removeDeadStates();
	}

	void onMessageReceived(const SendScriptMsgMessage& msg, ScriptableFamily& e) override
	{
		sendLocalMessage(e.entityId, msg.msg);
	}

	void onMessageReceived(const TerminateScriptsWithTagSystemMessage& msg) override
	{
		const auto* scriptable = scriptableFamily.tryFind(msg.scriptableId);
		if (scriptable == nullptr) {
			return;
		}

		auto& env = getScriptingService().getEnvironment();
		for (auto& state : scriptable->scriptable.activeStates) {
			if (state->hasTag(msg.tag)) {
				env.terminateState(*state, scriptable->entityId, scriptable->scriptable.variables);
			}
		}
		scriptable->scriptable.activeStates.removeDeadStates();
	}

	void onMessageReceived(const SetEntityVariableMessage& msg, ScriptableFamily& e) override
	{
		e.scriptable.variables.setVariable(msg.variable, ConfigNode(msg.value));
	}

	void sendReturnHostThread(EntityId target, const String& scriptId, int node, ConfigNode params) override
	{
		sendMessage(target, ReturnHostScriptThreadMessage(scriptId, node, std::move(params)));
	}

	void startHostThread(EntityId entityId, const String& scriptId, int nodeId, ConfigNode params) override
	{
		sendMessage(StartHostScriptThreadSystemMessage(scriptId, entityId, nodeId, std::move(params)));
	}

	void cancelHostThread(EntityId entityId, const String& scriptId, int nodeId) override
	{
		sendMessage(CancelHostScriptThreadSystemMessage(scriptId, entityId, nodeId));
	}

private:
	Vector<std::pair<EntityId, ScriptMessage>> pendingMessages;

	void initializeEnvironment()
	{
		getScriptingService().getEnvironment().setScriptTargetRetriever([this] (const String& id) -> EntityId
		{
			const auto* result = targetFamily.tryMatch([&] (const TargetFamily& e) { return e.scriptTarget.id == id; });
			return result ? result->entityId : EntityId();
		});
	}

	void initializeScripts()
	{
		for (auto& e: embeddedScriptFamily) {
			const bool running = std_ex::contains_if(e.scriptable.activeStates, [&] (const auto& kv)
			{
				return kv->getScriptGraphPtr() == &e.embeddedScript.script;
			});
			if (!running) {
				const auto entity = getWorld().getEntity(e.entityId);
				const auto& id = entity.getPrefabUUID().isValid() ? entity.getPrefabUUID() : entity.getInstanceUUID();
				e.embeddedScript.script.setAssetId(String("embed:") + id);

				addScript(e.entityId, e.scriptable, e.embeddedScript.script);
			}
		}

		for (auto& e: scriptableFamily) {
			if (getWorld().getEntity(e.entityId).isLocal()) {
				for (const auto& script : e.scriptable.scripts) {
					if (!std_ex::contains(e.scriptable.scriptsStarted, script.getAssetId())) {
						e.scriptable.scriptsStarted.push_back(script.getAssetId());
						const bool running = std_ex::contains_if(e.scriptable.activeStates, [&](const auto& kv)
						{
							return kv->getScriptGraphPtr() == script.get().get();
						});
						if (!running) {
							addScript(e.entityId, e.scriptable, script.get(), e.scriptable.tags);
						}
					}
				}
			}
			for (auto& state: e.scriptable.activeStates) {
				state->setFrameFlag(false);
			}
		}
	}

	void updateScripts(Time t)
	{
		auto& env = getScriptingService().getEnvironment();
		for (auto& e : scriptableFamily) {
			for (auto& state: e.scriptable.activeStates) {
				if (!state->getFrameFlag()) {
					env.update(t, *state, e.entityId, e.scriptable.variables);
					state->setFrameFlag(true);
				}
			}

			eraseDeadScripts(e);
		}
	}

	void eraseDeadScripts(ScriptableFamily& e)
	{
		e.scriptable.activeStates.removeDeadLocalStates(getWorld(), e.entityId);
	}

	bool fulfillScriptExecutionRequests()
	{
		bool addedAny = false;
		auto& env = getScriptingService().getEnvironment();
		auto requests = env.getScriptExecutionRequests();

		// Run all stop requests first (otherwise you might try to stop-start a script and it'll fail because it's already running)
		for (const auto& r: requests) {
			if (r.type == ScriptEnvironment::ScriptExecutionRequestType::Stop || r.type == ScriptEnvironment::ScriptExecutionRequestType::StopTag) {
				if (auto* scriptable = scriptableFamily.tryFind(r.target)) {
					for (auto& state: scriptable->scriptable.activeStates) {
						if ((r.type == ScriptEnvironment::ScriptExecutionRequestType::Stop && state->getScriptId() == r.value)
							|| (r.type == ScriptEnvironment::ScriptExecutionRequestType::StopTag && state->hasTag(r.value))) {
							getScriptingService().getEnvironment().stopState(*state, scriptable->entityId, scriptable->scriptable.variables, r.allThreads);
						}
					}
					eraseDeadScripts(*scriptable);
				}
			}
		}

		// Run start requests
		for (auto& r: requests) {
			if (r.type == ScriptEnvironment::ScriptExecutionRequestType::Start) {
				addScript(r.target, r.value, std::move(r.startTags), std::move(r.params));
				addedAny = true;
			}
		}

		return addedAny;
	}

	void sendMessages()
	{
		auto& env = getScriptingService().getEnvironment();
		auto scriptOutbound = env.getOutboundScriptMessages();
		for (auto& msg: scriptOutbound) {
			if (getWorld().isEntityNetworkRemote(msg.first)) {
				sendRemoteMessage(msg.first, std::move(msg.second));
			} else {
				sendLocalMessage(msg.first, std::move(msg.second));
			}
		}

		auto entityOutbound = env.getOutboundEntityMessages();
		for (auto& msg: entityOutbound) {
			getMessageBridge().sendMessageToEntity(msg.targetEntity, msg.messageName, msg.messageData);
		}
	}

	void sendRemoteMessage(EntityId dst, ScriptMessage msg)
	{
		sendMessage(dst, SendScriptMsgMessage(std::move(msg)));
	}

	void sendLocalMessage(EntityId dst, ScriptMessage msg)
	{
		if (msg.delay <= 0.00001f) {
			doSendLocalMessage(dst, std::move(msg));
		} else {
			pendingMessages.emplace_back(dst, std::move(msg));
		}
	}

	void updatePendingMessages(Time t)
	{
		for (auto& msg: pendingMessages) {
			msg.second.delay -= static_cast<float>(t);
			if (msg.second.delay <= 0.00001f) {
				doSendLocalMessage(msg.first, std::move(msg.second));
				msg.first = EntityId();
			}
		}
		std_ex::erase_if(pendingMessages, [=] (const auto& msg) { return !msg.first.isValid(); });
	}

	void doSendLocalMessage(EntityId dst, ScriptMessage msg)
	{
		const auto* scriptable = scriptableFamily.tryFind(dst);
		if (!scriptable) {
			return;
		}

		for (auto& script: scriptable->scriptable.activeStates) {
			if (script->getScriptId() == msg.type.script) {
				script->receiveMessage(std::move(msg));
				return;
			}
		}

		if (getResources().exists<ScriptGraph>(msg.type.script)) {
			auto script = getResources().get<ScriptGraph>(msg.type.script);
			getScriptingService().getEnvironment().assignTypes(*script);

			if (script->getMessageInboxId(msg.type.message, true)) {
				auto state = addScript(dst, scriptable->scriptable, script);
				state->receiveMessage(std::move(msg));
			}
		}
	}

	void addConsoleCommands()
	{
		getDevService().getConsoleCommands().addCommand("scriptRun", [=] (Vector<String> args) -> String
		{
			if (!args.empty() && args.size() <= 2) {
				if (!getResources().exists<ScriptGraph>(args[0])) {
					return "Script not found: " + args[0];
				}
				const auto script = getResources().get<ScriptGraph>(args[0]);
				const size_t n = runScript(script, args.size() >= 2 ? args[1] : "player");
				return "Attached script to " + toString(n) + " entities.";
			}
			return "Usage: scriptRun <scriptName> [tag=player]";
		});

		getDevService().getConsoleCommands().addCommand("eval", [=] (Vector<String> args) -> String
		{
			try {
				return getScriptingService().evaluateExpression(String::concatList(args, " ")).asString();
			} catch (const std::exception& e) {
				Logger::logException(e);
				return "Error";
			}
		});
	}

	size_t runScript(std::shared_ptr<const ScriptGraph> script, const String& tag)
	{
		size_t n = 0;
		for (auto& e: scriptableFamily) {
			if (std_ex::contains(e.scriptable.tags, tag)) {
				addScript(e.entityId, e.scriptable, std::move(script));
				++n;
			}
		}
		return n;
	}

	std::shared_ptr<ScriptState> addScript(EntityId entityId, ScriptableComponent& scriptable, std::shared_ptr<const ScriptGraph> script, Vector<String> tags = {}, Vector<ConfigNode> params = {})
	{
		if (hasScript(scriptable, script->getAssetId()) && !script->isMultiCopy()) {
			if (!script->isSupressDuplicateWarning()) {
				Logger::logWarning("Script " + script->getAssetId() + " already exists on entity " + getWorld().getEntity(entityId).getName());
			}
			return {};
		}
		return doAddScript(entityId, scriptable, std::make_shared<ScriptState>(std::move(script)), std::move(tags), std::move(params));
	}

	std::shared_ptr<ScriptState> addScript(EntityId entityId, ScriptableComponent& scriptable, const ScriptGraph& script, Vector<String> tags = {}, Vector<ConfigNode> params = {})
	{
		if (hasScript(scriptable, script.getAssetId()) && !script.isMultiCopy()) {
			if (!script.isSupressDuplicateWarning()) {
				Logger::logWarning("Script " + script.getAssetId() + " already exists on entity " + getWorld().getEntity(entityId).getName());
			}
			return {};
		}
		return doAddScript(entityId, scriptable, std::make_shared<ScriptState>(&script, true), std::move(tags), std::move(params));
	}

	std::shared_ptr<ScriptState> doAddScript(EntityId entityId, ScriptableComponent& scriptable, std::shared_ptr<ScriptState> state, Vector<String> tags, Vector<ConfigNode> params)
	{
		assert(!state->getScriptId().isEmpty());
		scriptable.activeStates.addState(state);
		state->setTags(std::move(tags));
		state->setStartParams(std::move(params));
		getScriptingService().getEnvironment().update(0, *state, entityId, scriptable.variables);
		return state;
	}

	bool hasScript(ScriptableComponent& scriptable, const String& id)
	{
		for (auto& state: scriptable.activeStates) {
			if (state->getScriptId() == id) {
				return true;
			}
		}
		return false;
	}

	void updateDevCon()
	{
		ProfilerEvent event(ProfilerEventType::CoreDevConClient, "Scripts");
		auto* devConClient = getAPI().core->getDevConClient();
		if (devConClient) {
			updateInterest(devConClient->getInterest());
		}
	}

	void updateInterest(DevConInterest& interest)
	{
		if (interest.hasInterest("scriptEnum")) {
			size_t i = 0;
			for (const auto& config : interest.getInterestConfigs("scriptEnum")) {
				ConfigNode::SequenceType result;
				const String& scriptId = config["scriptId"].asString();
				const uint64_t scriptHash = static_cast<uint64_t>(config["scriptHash"].asInt64());
				
				for (const auto& e : scriptableFamily) {
					for (const auto& state : e.scriptable.activeStates) {
						const auto* graph = state->getScriptGraphPtr();
						if (!graph || graph->getAssetHash() != scriptHash) {
							continue;
						}
						auto indices = graph->getSubGraphIndicesForAssetId(scriptId);

						for (auto scriptIdx: indices) {
							ConfigNode::MapType entry;
							entry["entityId"] = EntityIdHolder{ e.entityId.value };
							entry["name"] = getWorld().getEntity(e.entityId).getName();
							entry["scriptIdx"] = scriptIdx;
							result.push_back(entry);
							break;
						}
					}
				}

				interest.notifyInterest("scriptEnum", i, std::move(result));
				i++;
			}
		}

		if (interest.hasInterest("scriptState")) {
			size_t i = 0;
			for (const auto& config : interest.getInterestConfigs("scriptState")) {
				const String& scriptId = config["scriptId"].asString();
				const uint64_t scriptHash = static_cast<uint64_t>(config["scriptHash"].asInt64());
				const EntityId entityId = EntityId(config["entityId"].asEntityId().value);
				const int scriptIdx = config["scriptIdx"].asInt(0);

				const auto* e = scriptableFamily.tryFind(entityId);
				if (e) {
					for (const auto& state : e->scriptable.activeStates) {
						const auto* graph = state->getScriptGraphPtr();
						if (!graph || graph->getAssetHash() != scriptHash) {
							continue;
						}
						auto indices = graph->getSubGraphIndicesForAssetId(scriptId);

						if (scriptIdx >= 0 && scriptIdx < indices.size()) {
							auto subGraphIndex = indices[scriptIdx];
							const auto nodeRange = graph->getSubGraphRange(subGraphIndex);

							EntitySerializationContext context;
							context.entitySerializationTypeMask = EntitySerialization::makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::SaveData, EntitySerialization::Type::DevCon);
							context.resources = &getResources();
							context.entityContext = this;

							ConfigNode result = ConfigNode::MapType();
							result["scriptState"] = state->toConfigNode(context);
							result["nodeRange"] = Range<int>(nodeRange);

							ConfigNode::MapType variables{};
							variables["entity"] = e->scriptable.variables.toConfigNode(context);
							variables["local"] = state->getLocalVariables().toConfigNode(context);
							variables["shared"] = state->getSharedVariables().toConfigNode(context);
							result["variables"] = variables;

							const auto& scriptGraph = subGraphIndex == -1 ? *state->getScriptGraphPtr() : *getResources().get<ScriptGraph>(scriptId);
							result["roots"] = scriptGraph.getRoots().toConfigNode();

							if (config.hasKey("curNode")) {
								const GraphNodeId nodeId = config["curNode"]["nodeId"].asInt() + nodeRange.start;
								const GraphPinId elementId = config["curNode"]["elementId"].asInt();
								result["curNode"] = config["curNode"];
								result["curNode"]["value"] = getScriptingService().getEnvironment().readNodeElementDevConData(*state, entityId, e->scriptable.variables, nodeId, elementId);
							}
							
							ConfigNode::SequenceType debugDisplays;
							for (const auto& node: scriptGraph.getNodes()) {
								if (node.getNodeType().getClassification() == ScriptNodeClassification::DebugDisplay) {
									ConfigNode::MapType entry;
									entry["nodeId"] = node.getId();
									entry["value"] = getScriptingService().getEnvironment().readNodeElementDevConData(*state, entityId, e->scriptable.variables, node.getId(), 0);
									debugDisplays.push_back(std::move(entry));
								}
							}
							result["debugDisplays"] = std::move(debugDisplays);

							interest.notifyInterest("scriptState", i, std::move(result));
							break;
						}
					}
				}

				i++;
			}
		}
	}
	
	EntityId getEntityIdFromUUID(const UUID& uuid) const override
	{
		return getWorld().findEntity(uuid).value_or(EntityRef()).getEntityId();
	}

	UUID getUUIDFromEntityId(EntityId id) const override
	{
		auto e = getWorld().tryGetEntity(id);
		if (e.isValid()) {
			return e.getInstanceUUID();
		} else {
			return UUID();
		}
	}

	std::shared_ptr<ScriptState> getScriptState(EntityId entityId, const String& scriptName)
	{
		if (auto* e = scriptableFamily.tryFind(entityId)) {
			return getScriptState(*e, scriptName);
		}
		return {};
	}

	std::shared_ptr<ScriptState> getScriptState(ScriptableFamily& e, const String& scriptName)
	{
		for (const auto& scriptable: e.scriptable.activeStates) {
			if (scriptable->getScriptId() == scriptName) {
				return scriptable;
			}
		}
		return {};
	}
};

REGISTER_SYSTEM(ScriptSystem)

