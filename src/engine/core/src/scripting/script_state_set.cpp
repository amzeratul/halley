#include "halley/scripting/script_state_set.h"

#include "halley/scripting/script_environment.h"
#include "halley/scripting/script_graph.h"
#include "halley/scripting/script_state.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

void ScriptStateSet::load(const ConfigNode& node, const EntitySerializationContext& context)
{
	HalleyAssertDev(isValid());

	if (node.getType() == ConfigNodeType::Noop) {
		return;
	}

	const bool isNetwork = context.matchType(makeMask(EntitySerialization::Type::Network));

	if (node.hasKey("curId") && node["curId"].getType() != ConfigNodeType::Noop) {
		curId = node["curId"].asInt64(0);
	}

	if (node.hasKey("states") && node["states"].getType() != ConfigNodeType::Noop) {
		for (auto& state: states) {
			// For network updates, keep all local states (states attached to scripts
			// which do not set the "Synchronize over the network" property).
			//
			// Makes sure we do not mark them as "dead" down below.
			state.present = isNetwork && !state.state->getScriptGraphPtr()->isNetwork();
		}

		for (const auto& [stateId, stateNode]: node["states"].asMap()) {
			const auto id = stateId.toInteger64();

			if (isNetwork && stateNode.getType() == ConfigNodeType::Del) {
				if (auto* stateData = tryGetStateData(id)) {
					stateData->present = false;
				} else {
					// TODO:
					// Right now this can - unintentionally - happen if a network entity changes
					// in structure (child entities or components are added or removed). This case
					// triggers the "slow" update path using EntityDataDelta, which can work on
					// outdated information.
					Logger::logWarning("Want to delete non-existing script state " + toString(id) + "\n" + stateNode.asString());
				}
				continue;
			}

			const auto graph = context.resources->tryGet<ScriptGraph>(stateNode["script"].asString(""));
			const bool canLoad = graph && (isNetwork ? graph->isNetwork() : graph->isSerializableToSaveFile());
			if (!canLoad) {
				continue;
			}

			/*if (isNetwork && stateNode.getType() == ConfigNodeType::Noop) {
				if (auto* stateData = tryGetStateData(id)) {
					stateData->present = true;
				}
				continue;
			}*/

			auto& stateData = getStateData(id);
			stateData.present = true;

			if (stateData.state) {
				if (stateNode.getType() != ConfigNodeType::Noop) {
					stateData.state->load(stateNode, context);
				}
			} else {
				stateData.state = std::make_shared<ScriptState>(stateNode, context);
			}
		}

		for (auto& state: states) {
			if (!state.present) {
				state.dead = true;
				hasDeadStates = true;
			}
		}
	}

	HalleyAssertDev(isValid());
}

ConfigNode ScriptStateSet::toConfigNode(const EntitySerializationContext& context) const
{
	const bool isNetwork = context.matchType(makeMask(EntitySerialization::Type::Network));

	ConfigNode::MapType statesNode;
	for (auto& state: states) {
		const auto& script = state.state->getScriptGraphPtr();
		if (isNetwork ? script->isNetwork() : script->isSerializableToSaveFile()) {
			statesNode[toString(state.id)] = state.state->toConfigNode(context);
		}
	}

	ConfigNode::MapType result;
	result["states"] = std::move(statesNode);
	if (!isNetwork) {
		result["curId"] = curId;
	}
	return result;
}

void ScriptStateSet::feedToHasher(Hash::Hasher& hasher) const
{
	for (const auto& state: states) {
		state.feedToHasher(hasher);
	}
}

void ScriptStateSet::serialize(Serializer& s, const EntitySerializationContext& context) const
{
	if (s.getOptions().toHash) {
		const bool isNetwork = context.matchType(makeMask(EntitySerialization::Type::Network));
		for (auto& state: states) {
			if (!isNetwork || state.state->getScriptGraphPtr()->isNetwork()) {
				state.state->serialize(s, context);
			}
		}
		s << curId;
	} else {
		s << toConfigNode(context);
	}
}

void ScriptStateSet::deserialize(Deserializer& s, const EntitySerializationContext& context)
{
	ConfigNode node;
	s >> node;
	load(node, context);
}

void ScriptStateSet::addState(std::shared_ptr<ScriptState> state)
{
	HalleyAssertDev(!!state);
	states.push_back(State{ curId++, std::move(state) });
}

void ScriptStateSet::clear()
{
	states.clear();
}

bool ScriptStateSet::empty() const
{
	return states.empty();
}

void ScriptStateSet::removeDeadLocalStates(World& world, EntityId entityId)
{
	HalleyAssertDebug(isValid());
	// Defer evaluation to when it's needed to avoid querying the world too much
	// We check isDead first since isLocal is always true on single player, so it's unlikely to cut off a lot of work
	std::optional<bool> isLocalCache;
	auto isLocal = [&]() -> bool
	{
		if (!isLocalCache) {
			isLocalCache = world.getEntity(entityId).isNetworkAuthority();
		}
		return *isLocalCache;
	};

	std_ex::erase_if(states, [&](const auto& state)
	{
		return state.state->isDead() && isLocal();
	});
	HalleyAssertDebug(isValid());
}

void ScriptStateSet::terminateMarkedDead(ScriptEnvironment& environment, EntityId entityId, ScriptVariables& entityVariables)
{
	HalleyAssertDebug(isValid());
	if (hasDeadStates) {
		for (auto& state: states) {
			if (state.dead) {
				environment.terminateState(*state.state, entityId, entityVariables);
			}
		}
		hasDeadStates = false;
	}

	// We run this anyway because there are other ways states can be terminated
	removeDeadLocalStates(environment.getWorld(), entityId);
}

ScriptStateSet::Iterator ScriptStateSet::begin() const
{
	return Iterator(*this, 0);
}

ScriptStateSet::Iterator ScriptStateSet::end() const
{
	return Iterator(*this, states.size());
}

Vector<String> ScriptStateSet::getScriptIdList() const
{
	Vector<String> result;
	result.reserve(states.size());
	for (const auto& state: states) {
		if (state.state) {
			result += state.state->getScriptId();
		}
	}
	return result;
}

const std::shared_ptr<ScriptState>& ScriptStateSet::getState(size_t idx) const
{
	return states[idx].state;
}


void ScriptStateSet::State::feedToHasher(Hash::Hasher& hasher) const
{
	//hasher.feed(id);
	hasher.feed(present);
	hasher.feed(dead);
	if (state) {
		state->feedToHasher(hasher);
	}
}

ScriptStateSet::State& ScriptStateSet::getStateData(int64_t id)
{
	for (auto& state: states) {
		if (state.id == id) {
			return state;
		}
	}
	return states.emplace_back(State{ id, {}, true });
}

ScriptStateSet::State* ScriptStateSet::tryGetStateData(int64_t id)
{
	for (auto& state: states) {
		if (state.id == id) {
			return &state;
		}
	}
	return nullptr;
}

bool ScriptStateSet::isValid() const
{
	return std::all_of(states.begin(), states.end(), [&](const State& state) { return !!state.state; });
}

ConfigNode ConfigNodeSerializer<ScriptStateSet>::serialize(const ScriptStateSet& stateSet, const EntitySerializationContext& context)
{
	return stateSet.toConfigNode(context);
}

ScriptStateSet ConfigNodeSerializer<ScriptStateSet>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	ScriptStateSet stateSet;
	stateSet.load(node, context);
	return stateSet;
}

void ConfigNodeSerializer<ScriptStateSet>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptStateSet& target)
{
	target.load(node, context);
}

void ConfigNodeSerializer<ScriptStateSet>::hash(const ScriptStateSet& stateSet, Hash::Hasher& hasher)
{
	stateSet.feedToHasher(hasher);
}

void ByteSerializationHelper<ScriptStateSet>::serialize(const ScriptStateSet& value, const ByteSerializationContext& context, Serializer& serializer, int componentIndex, std::string_view fieldName)
{
    value.serialize(serializer, *context.entitySerializationContext);
}

void ByteSerializationHelper<ScriptStateSet>::deserialize(ScriptStateSet& dst, const ByteSerializationContext& context, Deserializer& deserializer, int componentIndex, std::string_view fieldName)
{
	dst.deserialize(deserializer, *context.entitySerializationContext);
}
