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
        ConfigNode toConfig() const;
    };

    class ScriptMessage {
    public:
        ScriptMessageType type;
        ConfigNode params;

        ScriptMessage() = default;
        ScriptMessage(const ConfigNode& node);
        ConfigNode toConfig() const;
        String toString() const;
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
