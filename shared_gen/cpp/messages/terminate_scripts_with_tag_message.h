// Halley codegen version 123
#pragma once

#include <halley.hpp>


class TerminateScriptsWithTagMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 5 };
	static const constexpr char* messageName{ "TerminateScriptsWithTag" };

	Halley::String tag{};

	TerminateScriptsWithTagMessage() {
	}

	TerminateScriptsWithTagMessage(Halley::String tag)
		: tag(std::move(tag))
	{
	}

	size_t getSize() const override final {
		return sizeof(TerminateScriptsWithTagMessage);
	}

	int getId() const override final {
		return messageIndex;
	}

	void serialize(Halley::Serializer& s) const override final {
		s << tag;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> tag;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(tag)>::deserialize(tag, Halley::String{}, context, node, "", "tag", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}
};
