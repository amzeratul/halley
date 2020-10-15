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

	Transform2DComponentBase(Halley::Vector2f position, Halley::Angle1f rotation, Halley::Vector2f scale, Halley::OptionalLite<int> subWorld)
		: position(std::move(position))
		, rotation(std::move(rotation))
		, scale(std::move(scale))
		, subWorld(std::move(subWorld))
	{
	}

	Halley::ConfigNode serialize(Halley::ConfigNodeSerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(position)>::serialize(position, context, node, "position", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(rotation)>::serialize(rotation, context, node, "rotation", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(scale)>::serialize(scale, context, node, "scale", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::serialize(subWorld, context, node, "subWorld", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(position)>::deserialize(position, context, node, "position", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(rotation)>::deserialize(rotation, context, node, "rotation", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(scale)>::deserialize(scale, context, node, "scale", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::deserialize(subWorld, context, node, "subWorld", makeMask(Type::Prefab, Type::SaveData));
	}

protected:
	Halley::Vector2f position{};
	Halley::Angle1f rotation{};
	Halley::Vector2f scale{ 1.0f, 1.0f };
	Halley::OptionalLite<int> subWorld{};

};
