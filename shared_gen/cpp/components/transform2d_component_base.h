// Halley codegen version 121
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class Transform2DComponentBase : public Halley::Component {
public:
	static constexpr int componentIndex{ 0 };
	static const constexpr char* componentName{ "Transform2D" };

	Transform2DComponentBase() {
	}

	Transform2DComponentBase(Halley::Vector2f position, Halley::Vector2f scale, Halley::Angle1f rotation, float height, Halley::OptionalLite<int16_t> subWorld)
		: position(std::move(position))
		, scale(std::move(scale))
		, rotation(std::move(rotation))
		, height(std::move(height))
		, subWorld(std::move(subWorld))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(position)>::serialize(position, Halley::Vector2f{}, _context, _node, componentName, "position", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scale)>::serialize(scale, Halley::Vector2f{ 1.0f, 1.0f }, _context, _node, componentName, "scale", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rotation)>::serialize(rotation, Halley::Angle1f{}, _context, _node, componentName, "rotation", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(height)>::serialize(height, float{}, _context, _node, componentName, "height", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::serialize(subWorld, Halley::OptionalLite<int16_t>{}, _context, _node, componentName, "subWorld", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(position)>::deserialize(position, Halley::Vector2f{}, _context, _node, componentName, "position", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scale)>::deserialize(scale, Halley::Vector2f{ 1.0f, 1.0f }, _context, _node, componentName, "scale", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rotation)>::deserialize(rotation, Halley::Angle1f{}, _context, _node, componentName, "rotation", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(height)>::deserialize(height, float{}, _context, _node, componentName, "height", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::deserialize(subWorld, Halley::OptionalLite<int16_t>{}, _context, _node, componentName, "subWorld", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

protected:
	Halley::Vector2f position{};
	Halley::Vector2f scale{ 1.0f, 1.0f };
	Halley::Angle1f rotation{};
	float height{};
	Halley::OptionalLite<int16_t> subWorld{};
	mutable uint16_t revision{ 0 };

};
