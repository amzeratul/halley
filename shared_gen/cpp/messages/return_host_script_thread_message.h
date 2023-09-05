// Halley codegen version 124
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ReturnHostScriptThreadMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 7 };
	static const constexpr char* messageName{ "ReturnHostScriptThread" };

	Halley::String script{};
	int nodeId{};

	ReturnHostScriptThreadMessage() {
	}

	ReturnHostScriptThreadMessage(Halley::String script, int nodeId)
		: script(std::move(script))
		, nodeId(std::move(nodeId))
	{
	}

	size_t getSize() const override final {
		return sizeof(ReturnHostScriptThreadMessage);
	}

	int getId() const override final {
		return messageIndex;
	}

	void serialize(Halley::Serializer& s) const override final {
		s << script;
		s << nodeId;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> script;
		s >> nodeId;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(script)>::deserialize(script, Halley::String{}, context, node, "", "script", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(nodeId)>::deserialize(nodeId, int{}, context, node, "", "nodeId", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}
};
