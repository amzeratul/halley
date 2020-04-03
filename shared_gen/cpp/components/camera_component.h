#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class CameraComponent final : public Halley::Component {
public:
	static constexpr int componentIndex = 4;
	
	float zoom = 1;
	std::optional<Halley::Colour4f> clear;
	int mask = 1;
	int layer = 0;
	
	CameraComponent() {}
	
	CameraComponent(float zoom, std::optional<Halley::Colour4f> clear, int mask, int layer)
		: zoom(zoom)
		, clear(clear)
		, mask(mask)
		, layer(layer)
	{}
	
	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper::deserializeIfDefined(zoom, context, node["zoom"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(clear, context, node["clear"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(mask, context, node["mask"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(layer, context, node["layer"]);
	}
};
