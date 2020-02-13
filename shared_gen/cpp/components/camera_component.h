#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class CameraComponent final : public Halley::Component {
public:
	static constexpr int componentIndex = 4;
	
	float zoom;
	Halley::Maybe<Halley::Colour4f> clear;
	int mask;
	int layer;
	
	CameraComponent() {}
	
	CameraComponent(float zoom, Halley::Maybe<Halley::Colour4f> clear, int mask, int layer)
		: zoom(zoom)
		, clear(clear)
		, mask(mask)
		, layer(layer)
	{}
	
	void deserialize(Halley::Resources& resources, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper::deserializeIfDefined(zoom, resources, node["zoom"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(clear, resources, node["clear"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(mask, resources, node["mask"]);
		Halley::ConfigNodeHelper::deserializeIfDefined(layer, resources, node["layer"]);
	}
};
