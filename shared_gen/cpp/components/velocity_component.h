// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class VelocityComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 1 };
	static const constexpr char* componentName{ "Velocity" };

	Halley::Vector2f velocity{};

	VelocityComponent() {
	}

	VelocityComponent(Halley::Vector2f velocity)
		: velocity(std::move(velocity))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(velocity)>::serialize(velocity, Halley::Vector2f{}, _context, _node, componentName, "velocity", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(velocity)>::deserialize(velocity, Halley::Vector2f{}, _context, _node, componentName, "velocity", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
