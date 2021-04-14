#pragma once
#include "halley/bytes/config_node_serializer.h"

namespace Halley {
    class ScriptState {
    public:
    	ScriptState();
		ScriptState(const ConfigNode& node);

		ConfigNode toConfigNode() const;
    };
	
	template<>
	class ConfigNodeSerializer<ScriptState> {
	public:
		ConfigNode serialize(const ScriptState& state, const ConfigNodeSerializationContext& context);
		ScriptState deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node);
	};

}
