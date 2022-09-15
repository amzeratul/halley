#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/bytes/config_node_serializer_base.h"

namespace Halley {
	class EntitySerializationContext;

	class ScriptVariables {
	public:
		ScriptVariables() = default;
		ScriptVariables(const ConfigNode& node, const EntitySerializationContext& context);

		void load(const ConfigNode& node, const EntitySerializationContext& context);
		ConfigNode toConfigNode(const EntitySerializationContext& context) const;

		const ConfigNode& getVariable(const String& name) const;
    	void setVariable(const String& name, ConfigNode value);
		bool hasVariable(const String& name) const;

		bool empty() const;
		void clear();

	private:
		ConfigNode dummy;
		HashMap<String, ConfigNode> variables;
	};

	template <>
    class ConfigNodeSerializer<ScriptVariables> {
    public:
		ConfigNode serialize(const ScriptVariables& variables, const EntitySerializationContext& context);
		ScriptVariables deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, ScriptVariables& target);
    };
}
