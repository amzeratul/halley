// Halley codegen version 125
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif
#include "halley/support/exception.h"


class NetworkComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 15 };
	static const constexpr char* componentName{ "Network" };

	std::optional<uint8_t> ownerId{};
	Halley::DataInterpolatorSet dataInterpolatorSet{};
	Halley::Vector<std::pair<Halley::EntityId, uint8_t>> locks{};
	bool sendUpdates{ false };

	NetworkComponent() {
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(locks)>::serialize(locks, Halley::Vector<std::pair<Halley::EntityId, uint8_t>>{}, _context, _node, componentName, "locks", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sendUpdates)>::serialize(sendUpdates, bool{ false }, _context, _node, componentName, "sendUpdates", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(locks)>::deserialize(locks, Halley::Vector<std::pair<Halley::EntityId, uint8_t>>{}, _context, _node, componentName, "locks", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(sendUpdates)>::deserialize(sendUpdates, bool{ false }, _context, _node, componentName, "sendUpdates", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "sendUpdates") {
			return Halley::ConfigNodeHelper<decltype(sendUpdates)>::serialize(sendUpdates, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "sendUpdates") {
			Halley::ConfigNodeHelper<decltype(sendUpdates)>::deserialize(sendUpdates, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

};
