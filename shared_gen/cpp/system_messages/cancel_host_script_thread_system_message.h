// Halley codegen version 124
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class CancelHostScriptThreadSystemMessage final : public Halley::SystemMessage {
public:
	static constexpr int messageIndex{ 3 };
	static const constexpr char* messageName{ "CancelHostScriptThread" };
	static constexpr Halley::SystemMessageDestination messageDestination{ Halley::SystemMessageDestination::Host };
	using ReturnType = void;

	Halley::String script{};
	Halley::EntityId entity{};
	int nodeId{};

	CancelHostScriptThreadSystemMessage() {
	}

	CancelHostScriptThreadSystemMessage(Halley::String script, Halley::EntityId entity, int nodeId)
		: script(std::move(script))
		, entity(std::move(entity))
		, nodeId(std::move(nodeId))
	{
	}

	size_t getSize() const override final {
		return sizeof(CancelHostScriptThreadSystemMessage);
	}

	int getId() const override final {
		return messageIndex;
	}
	Halley::SystemMessageDestination getMessageDestination() const override final {
		return messageDestination;
	}


	void serialize(Halley::Serializer& s) const override final {
		s << script;
		s << entity;
		s << nodeId;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> script;
		s >> entity;
		s >> nodeId;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(script)>::deserialize(script, Halley::String{}, context, node, "", "script", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(entity)>::deserialize(entity, Halley::EntityId{}, context, node, "", "entity", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(nodeId)>::deserialize(nodeId, int{}, context, node, "", "nodeId", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}
};
