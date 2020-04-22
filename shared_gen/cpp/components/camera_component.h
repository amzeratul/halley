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
		: zoom(std::move(zoom))
		, clear(std::move(clear))
		, mask(std::move(mask))
		, layer(std::move(layer))
	{}
	
	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper<decltype(zoom)>::deserialize(zoom, context, node["zoom"]);
		Halley::ConfigNodeHelper<decltype(clear)>::deserialize(clear, context, node["clear"]);
		Halley::ConfigNodeHelper<decltype(mask)>::deserialize(mask, context, node["mask"]);
		Halley::ConfigNodeHelper<decltype(layer)>::deserialize(layer, context, node["layer"]);
	}
};
