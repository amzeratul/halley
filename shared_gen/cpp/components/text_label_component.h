#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class TextLabelComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 2 };
	
	Halley::TextRenderer text{};
	int layer{ 0 };
	Halley::OptionalLite<int> mask{};
	
	TextLabelComponent() {}
	
	TextLabelComponent(Halley::TextRenderer text, int layer, Halley::OptionalLite<int> mask)
		: text(std::move(text))
		, layer(std::move(layer))
		, mask(std::move(mask))
	{}
	
	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper<decltype(text)>::deserialize(text, context, node["text"]);
		Halley::ConfigNodeHelper<decltype(layer)>::deserialize(layer, context, node["layer"]);
		Halley::ConfigNodeHelper<decltype(mask)>::deserialize(mask, context, node["mask"]);
	}
};
