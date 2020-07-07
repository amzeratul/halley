#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SpriteAnimationComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 3 };
	static const constexpr char* componentName{ "SpriteAnimation" };

	Halley::AnimationPlayer player{};

	SpriteAnimationComponent() {
	}

	SpriteAnimationComponent(Halley::AnimationPlayer player)
		: player(std::move(player))
	{
	}

	Halley::ConfigNode serialize(Halley::ConfigNodeSerializationContext& context) const {
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		node["player"] = Halley::ConfigNodeHelper<decltype(player)>::serialize(player, context);
		return node;
	}

	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper<decltype(player)>::deserialize(player, context, node["player"]);
	}

};
