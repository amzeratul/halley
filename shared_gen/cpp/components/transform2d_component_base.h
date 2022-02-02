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

	Transform2DComponentBase(Halley::Vector2f position, Halley::Vector2f scale, Halley::Angle1f rotation, Halley::OptionalLite<int> subWorld)
		: position(std::move(position))
		, scale(std::move(scale))
		, rotation(std::move(rotation))
		, subWorld(std::move(subWorld))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(position)>::serialize(position, Halley::Vector2f{}, context, node, "position", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(scale)>::serialize(scale, Halley::Vector2f{ 1.0f, 1.0f }, context, node, "scale", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(rotation)>::serialize(rotation, Halley::Angle1f{}, context, node, "rotation", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::serialize(subWorld, Halley::OptionalLite<int>{}, context, node, "subWorld", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(position)>::deserialize(position, Halley::Vector2f{}, context, node, "position", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(scale)>::deserialize(scale, Halley::Vector2f{ 1.0f, 1.0f }, context, node, "scale", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(rotation)>::deserialize(rotation, Halley::Angle1f{}, context, node, "rotation", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::deserialize(subWorld, Halley::OptionalLite<int>{}, context, node, "subWorld", makeMask(Type::Prefab, Type::SaveData));
	}

protected:
	Halley::Vector2f position{};
	Halley::Vector2f scale{ 1.0f, 1.0f };
	Halley::Angle1f rotation{};
	Halley::OptionalLite<int> subWorld{};

};
