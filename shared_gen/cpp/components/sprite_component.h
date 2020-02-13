#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SpriteComponent final : public Halley::Component {
public:
	static constexpr int componentIndex = 0;
	
	Halley::Sprite sprite;
	int layer;
	int mask;
	
	SpriteComponent() {}
	
	SpriteComponent(Halley::Sprite sprite, int layer, int mask)
		: sprite(sprite)
		, layer(layer)
		, mask(mask)
	{}
	
	void deserialize(Halley::Resources& resources, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper::deserializeIfDefined(sprite, resources, node["sprite"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(layer, resources, node["layer"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(mask, resources, node["mask"]);
	}
};
