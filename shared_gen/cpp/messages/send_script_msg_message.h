// Halley codegen version 125
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SendScriptMsgMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 6 };
	static const constexpr char* messageName{ "SendScriptMsg" };

	Halley::ScriptMessage msg{};

	SendScriptMsgMessage() {
	}

	SendScriptMsgMessage(Halley::ScriptMessage msg)
		: msg(std::move(msg))
	{
	}

	size_t getSize() const override final {
		return sizeof(SendScriptMsgMessage);
	}

	int getId() const override final {
		return messageIndex;
	}

	void serialize(Halley::Serializer& s) const override final {
		s << msg;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> msg;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(msg)>::deserialize(msg, Halley::ScriptMessage{}, context, node, "", "msg", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
	}
};
