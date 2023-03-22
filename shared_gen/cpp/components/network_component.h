// Halley codegen version 120
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class NetworkComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 11 };
	static const constexpr char* componentName{ "Network" };

	std::optional<uint8_t> ownerId{};
	Halley::DataInterpolatorSet dataInterpolatorSet{};
	std::optional<uint8_t> lockOwnerId{};
	bool sendUpdates{ false };

	NetworkComponent() {
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(lockOwnerId)>::serialize(lockOwnerId, std::optional<uint8_t>{}, context, node, componentName, "lockOwnerId", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sendUpdates)>::serialize(sendUpdates, bool{ false }, context, node, componentName, "sendUpdates", makeMask(Type::SaveData, Type::Network));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(lockOwnerId)>::deserialize(lockOwnerId, std::optional<uint8_t>{}, context, node, componentName, "lockOwnerId", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sendUpdates)>::deserialize(sendUpdates, bool{ false }, context, node, componentName, "sendUpdates", makeMask(Type::SaveData, Type::Network));
	}

};
