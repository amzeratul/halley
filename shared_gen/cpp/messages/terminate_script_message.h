// Halley codegen version 125
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class TerminateScriptMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 4 };
	static const constexpr char* messageName{ "TerminateScript" };

	Halley::String name{};

	TerminateScriptMessage() {
	}

	TerminateScriptMessage(Halley::String name)
		: name(std::move(name))
	{
	}

	size_t getSize() const override final {
		return sizeof(TerminateScriptMessage);
	}

	int getId() const override final {
		return messageIndex;
	}

	void serialize(Halley::Serializer& s) const override final {
		s << name;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> name;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(name)>::deserialize(name, Halley::String{}, context, node, "", "name", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
	}
};
