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
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(player)>::serialize(player, Halley::AnimationPlayer{}, context, node, "player", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(player)>::deserialize(player, context, node, "player", makeMask(Type::Prefab, Type::SaveData));
	}

};
