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
	int subWorld = INT_MIN;
	
public:
	Transform2DComponentBase() {}
	
	Transform2DComponentBase(Halley::Vector2f position, Halley::Angle1f rotation, Halley::Vector2f scale, int subWorld)
		: position(position)
		, rotation(rotation)
		, scale(scale)
		, subWorld(subWorld)
	{}
	
	void deserialize(Halley::Resources& resources, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper::deserializeIfDefined(position, resources, node["position"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(rotation, resources, node["rotation"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(scale, resources, node["scale"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(subWorld, resources, node["subWorld"]);
	}
};
