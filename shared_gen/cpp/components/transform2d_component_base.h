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
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		node["position"] = Halley::ConfigNodeHelper<decltype(position)>::serialize(position, context);
		node["rotation"] = Halley::ConfigNodeHelper<decltype(rotation)>::serialize(rotation, context);
		node["scale"] = Halley::ConfigNodeHelper<decltype(scale)>::serialize(scale, context);
		node["subWorld"] = Halley::ConfigNodeHelper<decltype(subWorld)>::serialize(subWorld, context);
		return node;
	}

	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper<decltype(position)>::deserialize(position, context, node["position"]);
		Halley::ConfigNodeHelper<decltype(rotation)>::deserialize(rotation, context, node["rotation"]);
		Halley::ConfigNodeHelper<decltype(scale)>::deserialize(scale, context, node["scale"]);
		Halley::ConfigNodeHelper<decltype(subWorld)>::deserialize(subWorld, context, node["subWorld"]);
	}

protected:
	Halley::Vector2f position{};
	Halley::Angle1f rotation{};
	Halley::Vector2f scale{ 1.0f, 1.0f };
	Halley::OptionalLite<int> subWorld{};

};
