// Halley codegen version 127
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SetEntityVariableMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 8 };
	static const constexpr char* messageName{ "SetEntityVariable" };

	Halley::String variable{};
	Halley::ConfigNode value{};

	SetEntityVariableMessage() {
	}

	SetEntityVariableMessage(Halley::String variable, Halley::ConfigNode value)
		: variable(std::move(variable))
		, value(std::move(value))
	{
	}

	size_t getSize() const override final {
		return sizeof(SetEntityVariableMessage);
	}

	int getId() const override final {
		return messageIndex;
	}

	void serialize(Halley::Serializer& s) const override final {
		s << variable;
		s << value;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> variable;
		s >> value;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(variable)>::deserialize(variable, Halley::String{}, context, node, "", "variable", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(value)>::deserialize(value, Halley::ConfigNode{}, context, node, "", "value", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
	}
};
