// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SpriteAnimationComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 5 };
	static const constexpr char* componentName{ "SpriteAnimation" };

	Halley::AnimationPlayer player{};

	SpriteAnimationComponent() {
	}

	SpriteAnimationComponent(Halley::AnimationPlayer player)
		: player(std::move(player))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(player)>::serialize(player, Halley::AnimationPlayer{}, _context, _node, componentName, "player", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(player)>::deserialize(player, Halley::AnimationPlayer{}, _context, _node, componentName, "player", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
