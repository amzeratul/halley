// Halley codegen version 121
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class NetworkComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 12 };
	static const constexpr char* componentName{ "Network" };

	std::optional<uint8_t> ownerId{};
	Halley::DataInterpolatorSet dataInterpolatorSet{};
	Halley::HashMap<Halley::EntityId, uint8_t> locks{};
	bool sendUpdates{ false };

	NetworkComponent() {
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(locks)>::serialize(locks, Halley::HashMap<Halley::EntityId, uint8_t>{}, _context, _node, componentName, "locks", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sendUpdates)>::serialize(sendUpdates, bool{ false }, _context, _node, componentName, "sendUpdates", makeMask(Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(locks)>::deserialize(locks, Halley::HashMap<Halley::EntityId, uint8_t>{}, _context, _node, componentName, "locks", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sendUpdates)>::deserialize(sendUpdates, bool{ false }, _context, _node, componentName, "sendUpdates", makeMask(Type::SaveData, Type::Network));
	}

};
