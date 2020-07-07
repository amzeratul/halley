#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class CameraComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 4 };
	static const constexpr char* componentName{ "Camera" };

	float zoom{ 1 };
	std::optional<Halley::Colour4f> clear{};
	int mask{ 1 };
	int layer{ 0 };

	CameraComponent() {
	}

	CameraComponent(float zoom, std::optional<Halley::Colour4f> clear, int mask, int layer)
		: zoom(std::move(zoom))
		, clear(std::move(clear))
		, mask(std::move(mask))
		, layer(std::move(layer))
	{
	}

	Halley::ConfigNode serialize(Halley::ConfigNodeSerializationContext& context) const {
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		node["zoom"] = Halley::ConfigNodeHelper<decltype(zoom)>::serialize(zoom, context);
		node["clear"] = Halley::ConfigNodeHelper<decltype(clear)>::serialize(clear, context);
		node["mask"] = Halley::ConfigNodeHelper<decltype(mask)>::serialize(mask, context);
		node["layer"] = Halley::ConfigNodeHelper<decltype(layer)>::serialize(layer, context);
		return node;
	}

	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper<decltype(zoom)>::deserialize(zoom, context, node["zoom"]);
		Halley::ConfigNodeHelper<decltype(clear)>::deserialize(clear, context, node["clear"]);
		Halley::ConfigNodeHelper<decltype(mask)>::deserialize(mask, context, node["mask"]);
		Halley::ConfigNodeHelper<decltype(layer)>::deserialize(layer, context, node["layer"]);
	}

};
