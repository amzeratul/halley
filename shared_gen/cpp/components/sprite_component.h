#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SpriteComponent final : public Halley::Component {
public:
	static constexpr int componentIndex = 1;
	
	Halley::Sprite sprite;
	int layer = 0;
	int mask = -1;
	
	SpriteComponent() {}
	
	SpriteComponent(Halley::Sprite sprite, int layer, int mask)
		: sprite(sprite)
		, layer(layer)
		, mask(mask)
	{}
	
	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper::deserializeIfDefined(sprite, context, node["sprite"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(layer, context, node["layer"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(mask, context, node["mask"]);
	}
};
