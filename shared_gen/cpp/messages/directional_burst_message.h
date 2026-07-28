// Halley codegen version 140
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class DirectionalBurstMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 3 };
	static const constexpr char* messageName{ "DirectionalBurst" };

	Halley::Vector2f direction{};
	int count{};

	DirectionalBurstMessage() {
	}

	DirectionalBurstMessage(Halley::Vector2f direction, int count)
		: direction(std::move(direction))
		, count(std::move(count))
	{
	}

	size_t getSize() const override final {
		return sizeof(DirectionalBurstMessage);
	}

	int getId() const override final {
		return messageIndex;
	}

	void serialize(Halley::Serializer& s) const override final {
		s << direction;
		s << count;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> direction;
		s >> count;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(direction)>::deserialize(direction, Halley::Vector2f{}, context, node, "", "direction", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(count)>::deserialize(count, int{}, context, node, "", "count", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
	}
};
