#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SpriteComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 1 };
	static const constexpr char* componentName{ "Sprite" };

	Halley::Sprite sprite{};
	int layer{ 0 };
	Halley::OptionalLite<int> mask{};

	SpriteComponent() {
	}

	SpriteComponent(Halley::Sprite sprite, int layer, Halley::OptionalLite<int> mask)
		: sprite(std::move(sprite))
		, layer(std::move(layer))
		, mask(std::move(mask))
	{
	}

	Halley::ConfigNode serialize(Halley::ConfigNodeSerializationContext& context) const {
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		node["sprite"] = Halley::ConfigNodeHelper<decltype(sprite)>::serialize(sprite, context);
		node["layer"] = Halley::ConfigNodeHelper<decltype(layer)>::serialize(layer, context);
		node["mask"] = Halley::ConfigNodeHelper<decltype(mask)>::serialize(mask, context);
		return node;
	}

	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper<decltype(sprite)>::deserialize(sprite, context, node["sprite"]);
		Halley::ConfigNodeHelper<decltype(layer)>::deserialize(layer, context, node["layer"]);
		Halley::ConfigNodeHelper<decltype(mask)>::deserialize(mask, context, node["mask"]);
	}

};
