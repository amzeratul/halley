// Halley codegen version 122
#pragma once

#include <halley.hpp>


class StartScriptMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 0 };
	static const constexpr char* messageName{ "StartScript" };

	Halley::String name{};
	Halley::Vector<Halley::String> tags{};
	Halley::Vector<Halley::ConfigNode> params{};

	StartScriptMessage() {
	}

	StartScriptMessage(Halley::String name, Halley::Vector<Halley::String> tags, Halley::Vector<Halley::ConfigNode> params)
		: name(std::move(name))
		, tags(std::move(tags))
		, params(std::move(params))
	{
	}

	size_t getSize() const override final {
		return sizeof(StartScriptMessage);
	}

	int getId() const override final {
		return messageIndex;
	}

	void serialize(Halley::Serializer& s) const override final {
		s << name;
		s << tags;
		s << params;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> name;
		s >> tags;
		s >> params;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(name)>::deserialize(name, Halley::String{}, context, node, "", "name", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::deserialize(tags, Halley::Vector<Halley::String>{}, context, node, "", "tags", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(params)>::deserialize(params, Halley::Vector<Halley::ConfigNode>{}, context, node, "", "params", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}
};
