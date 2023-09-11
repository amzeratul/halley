// Halley codegen version 125
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
	Halley::ConfigNode params{};

	ReturnHostScriptThreadMessage() {
	}

	ReturnHostScriptThreadMessage(Halley::String script, int nodeId, Halley::ConfigNode params)
		: script(std::move(script))
		, nodeId(std::move(nodeId))
		, params(std::move(params))
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
		s << params;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> script;
		s >> nodeId;
		s >> params;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(script)>::deserialize(script, Halley::String{}, context, node, "", "script", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(nodeId)>::deserialize(nodeId, int{}, context, node, "", "nodeId", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(params)>::deserialize(params, Halley::ConfigNode{}, context, node, "", "params", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
	}
};
