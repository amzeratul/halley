#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class Transform2DComponentBase : public Halley::Component {
public:
	static constexpr int componentIndex = 0;
	
protected:
	Halley::Vector2f position;
	Halley::Angle1f rotation;
	Halley::Vector2f scale = Halley::Vector2f(1.0f, 1.0f);
	Halley::OptionalLite<int> subWorld;
	
public:
	Transform2DComponentBase() {}
	
	Transform2DComponentBase(Halley::Vector2f position, Halley::Angle1f rotation, Halley::Vector2f scale, Halley::OptionalLite<int> subWorld)
		: position(position)
		, rotation(rotation)
		, scale(scale)
		, subWorld(subWorld)
	{}
	
	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper<decltype(position)>::deserialize(position, context, node["position"]);
		Halley::ConfigNodeHelper<decltype(rotation)>::deserialize(rotation, context, node["rotation"]);
		Halley::ConfigNodeHelper<decltype(scale)>::deserialize(scale, context, node["scale"]);
		Halley::ConfigNodeHelper<decltype(subWorld)>::deserialize(subWorld, context, node["subWorld"]);
	}
};
