#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SpriteAnimationComponent final : public Halley::Component {
public:
	static constexpr int componentIndex = 3;
	
	Halley::AnimationPlayer player;
	
	SpriteAnimationComponent() {}
	
	SpriteAnimationComponent(Halley::AnimationPlayer player)
		: player(player)
	{}
	
	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper<decltype(player)>::deserialize(player, context, node["player"]);
	}
};
