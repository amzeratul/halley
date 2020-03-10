#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class TextLabelComponent final : public Halley::Component {
public:
	static constexpr int componentIndex = 2;
	
	Halley::TextRenderer text;
	int layer;
	int mask;
	
	TextLabelComponent() {}
	
	TextLabelComponent(Halley::TextRenderer text, int layer, int mask)
		: text(text)
		, layer(layer)
		, mask(mask)
	{}
	
	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper::deserializeIfDefined(text, context, node["text"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(layer, context, node["layer"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(mask, context, node["mask"]);
	}
};
