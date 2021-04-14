#include "halley/core/scripting/script_state.h"
using namespace Halley;

ScriptState::ScriptState()
{}

ScriptState::ScriptState(const ConfigNode& node)
{
	// TODO
}

ConfigNode ScriptState::toConfigNode() const
{
	ConfigNode::MapType result;
	// TODO
	return result;
}

ConfigNode ConfigNodeSerializer<ScriptState>::serialize(const ScriptState& state, const ConfigNodeSerializationContext& context)
{
	return state.toConfigNode();
}

ScriptState ConfigNodeSerializer<ScriptState>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	return ScriptState(node);
}
