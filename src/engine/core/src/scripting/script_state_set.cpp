#include "halley/scripting/script_state_set.h"

#include "halley/scripting/script_environment.h"
#include "halley/scripting/script_graph.h"
#include "halley/scripting/script_state.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

void ScriptStateSet::load(const ConfigNode& node, const EntitySerializationContext& context)
{
	assert(isValid());

	if (node.getType() == ConfigNodeType::Noop) {
		return;
	}

	if (node.hasKey("curId") && node["curId"].getType() != ConfigNodeType::Noop) {
		curId = node["curId"].asInt64(0);
	}

	if (node.hasKey("states") && node["states"].getType() != ConfigNodeType::Noop) {
		for (auto& state: states) {
			state.present = false;
		}

		for (const auto& [stateId, stateNode]: node["states"].asMap()) {
			const auto id = stateId.toInteger64();
			if (stateNode.getType() == ConfigNodeType::Del) {
				continue;
			}

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
			}
		}
	}

	assert(isValid());
}

ConfigNode ScriptStateSet::toConfigNode(const EntitySerializationContext& context) const
{
	const bool isNetwork = context.matchType(EntitySerialization::makeMask(EntitySerialization::Type::Network));

	ConfigNode::MapType statesNode;
	for (auto& state: states) {
		if (!isNetwork || state.state->getScriptGraphPtr()->isNetwork()) {
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

void ScriptStateSet::addState(std::shared_ptr<ScriptState> state)
{
	assert(!!state);
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
	assert(isValid());
	// Defer evaluation to when it's needed to avoid querying the world too much
	std::optional<bool> isLocalCache;
	auto isLocal = [&]() -> bool
	{
		if (!isLocalCache) {
			isLocalCache = world.getEntity(entityId).isLocal();
		}
		return *isLocalCache;
	};

	std_ex::erase_if(states, [&](const auto& state)
	{
		return state.state->isDead() && isLocal();
	});
	assert(isValid());
}

void ScriptStateSet::terminateMarkedDead(ScriptEnvironment& environment, EntityId entityId, ScriptVariables& entityVariables)
{
	assert(isValid());
	for (auto& state: states) {
		if (state.dead) {
			environment.terminateState(*state.state, entityId, entityVariables);
		}
	}
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

const std::shared_ptr<ScriptState>& ScriptStateSet::getState(size_t idx) const
{
	return states[idx].state;
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
