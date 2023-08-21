#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/text/halleystring.h"
#include "halley/bytes/config_node_serializer_base.h"

namespace Halley {
	class ScriptMessageType {
    public:
        String script;
        String message;
        int nParams = 0;

        ScriptMessageType() = default;
        ScriptMessageType(const ConfigNode& node);
        ScriptMessageType(String script, String message, int nParams);
        ConfigNode toConfig() const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);
    };

    class ScriptMessage {
    public:
        ScriptMessageType type;
        ConfigNode params;
        float delay = 0;

        ScriptMessage() = default;
        ScriptMessage(String script, String message);
        ScriptMessage(String script, String message, ConfigNode params);
    	ScriptMessage(const ConfigNode& node);
        ConfigNode toConfig() const;
        String toString() const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);
    };

	class ScriptEntityMessageType {
	public:
        String message;
        Vector<String> members;

        ScriptEntityMessageType() = default;
        ScriptEntityMessageType(const ConfigNode& node);
        ConfigNode toConfig() const;
	};

    using ScriptSystemMessageType = ScriptEntityMessageType;
	
	template<>
	class ConfigNodeSerializer<ScriptMessage> {
	public:
		ConfigNode serialize(const ScriptMessage& msg, const EntitySerializationContext& context);
		ScriptMessage deserialize(const EntitySerializationContext& context, const ConfigNode& node);
	};
}
