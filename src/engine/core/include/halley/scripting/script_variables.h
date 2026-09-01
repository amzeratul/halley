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
		ConfigNode toConfigNode() const;
		ConfigNode toConfigNode(const EntitySerializationContext& context) const;
		void feedToHasher(Hash::Hasher& hasher, const EntitySerializationContext& context) const;

		void serialize(Serializer& s, const EntitySerializationContext& context) const;
		void deserialize(Deserializer& s, const EntitySerializationContext& context);

		const ConfigNode& getVariable(std::string_view name) const;
    	void setVariable(std::string_view name, ConfigNode value);
		bool hasVariable(std::string_view name) const;

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
		void hash(const ScriptVariables& variables, const EntitySerializationContext& context, Hash::Hasher& hasher);
    };

    template <typename T>
    class ByteSerializationHelper;
	class ByteSerializationContext;

    template <>
    class ByteSerializationHelper<ScriptVariables> {
    public:
        static void serialize(const ScriptVariables& value, const ByteSerializationContext& context, Serializer& serializer, int componentIndex, std::string_view fieldName);
        static void deserialize(ScriptVariables& dst, const ByteSerializationContext& context, Deserializer& deserializer, int componentIndex, std::string_view fieldName);
    };
}
