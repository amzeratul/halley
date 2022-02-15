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

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(sprite)>::serialize(sprite, Halley::Sprite{}, context, node, "sprite", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, int{ 0 }, context, node, "layer", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, Halley::OptionalLite<int>{}, context, node, "mask", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(sprite)>::deserialize(sprite, Halley::Sprite{}, context, node, "sprite", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, int{ 0 }, context, node, "layer", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, Halley::OptionalLite<int>{}, context, node, "mask", makeMask(Type::Prefab, Type::SaveData));
	}

};
