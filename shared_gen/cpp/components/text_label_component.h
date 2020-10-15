#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class TextLabelComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 2 };
	static const constexpr char* componentName{ "TextLabel" };

	Halley::TextRenderer text{};
	int layer{ 0 };
	Halley::OptionalLite<int> mask{};

	TextLabelComponent() {
	}

	TextLabelComponent(Halley::TextRenderer text, int layer, Halley::OptionalLite<int> mask)
		: text(std::move(text))
		, layer(std::move(layer))
		, mask(std::move(mask))
	{
	}

	Halley::ConfigNode serialize(Halley::ConfigNodeSerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(text)>::serialize(text, context, node, "text", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, context, node, "layer", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, context, node, "mask", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(text)>::deserialize(text, context, node, "text", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, context, node, "layer", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, context, node, "mask", makeMask(Type::Prefab, Type::SaveData));
	}

};
