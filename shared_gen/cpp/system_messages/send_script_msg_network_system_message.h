// Halley codegen version 140
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SendScriptMsgNetworkSystemMessage final : public Halley::SystemMessage {
public:
	static constexpr int messageIndex{ 3 };
	static const constexpr char* messageName{ "SendScriptMsgNetwork" };
	static constexpr Halley::SystemMessageDestination messageDestination{ Halley::SystemMessageDestination::Host };
	using ReturnType = void;

	Halley::EntityId entityId{};
	Halley::ScriptMessage msg{};

	SendScriptMsgNetworkSystemMessage() {
	}

	SendScriptMsgNetworkSystemMessage(Halley::EntityId entityId, Halley::ScriptMessage msg)
		: entityId(std::move(entityId))
		, msg(std::move(msg))
	{
	}

	size_t getSize() const override final {
		return sizeof(SendScriptMsgNetworkSystemMessage);
	}

	int getId() const override final {
		return messageIndex;
	}
	Halley::SystemMessageDestination getMessageDestination() const override final {
		return messageDestination;
	}


	void serialize(Halley::Serializer& s) const override final {
		s << entityId;
		s << msg;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> entityId;
		s >> msg;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(entityId)>::deserialize(entityId, Halley::EntityId{}, context, node, "", "entityId", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(msg)>::deserialize(msg, Halley::ScriptMessage{}, context, node, "", "msg", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
	}
};
