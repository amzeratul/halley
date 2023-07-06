// Halley codegen version 121
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

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(text)>::serialize(text, Halley::TextRenderer{}, _context, _node, componentName, "text", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, Halley::OptionalLite<int>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(text)>::deserialize(text, Halley::TextRenderer{}, _context, _node, componentName, "text", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, Halley::OptionalLite<int>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
