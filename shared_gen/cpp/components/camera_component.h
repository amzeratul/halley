#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class CameraComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 4 };
	static const constexpr char* componentName{ "Camera" };

	float zoom{ 1 };
	std::optional<Halley::Colour4f> clearColour{};
	std::optional<int> clearStencil{};
	std::optional<float> clearDepth{};
	int mask{ 1 };
	int layer{ 0 };
	Halley::RenderTarget* target{ nullptr };

	CameraComponent() {
	}

	CameraComponent(float zoom, std::optional<Halley::Colour4f> clearColour, std::optional<int> clearStencil, std::optional<float> clearDepth, int mask, int layer)
		: zoom(std::move(zoom))
		, clearColour(std::move(clearColour))
		, clearStencil(std::move(clearStencil))
		, clearDepth(std::move(clearDepth))
		, mask(std::move(mask))
		, layer(std::move(layer))
	{
	}

	Halley::ConfigNode serialize(Halley::ConfigNodeSerializationContext& context) const {
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		node["zoom"] = Halley::ConfigNodeHelper<decltype(zoom)>::serialize(zoom, context);
		node["clearColour"] = Halley::ConfigNodeHelper<decltype(clearColour)>::serialize(clearColour, context);
		node["clearStencil"] = Halley::ConfigNodeHelper<decltype(clearStencil)>::serialize(clearStencil, context);
		node["clearDepth"] = Halley::ConfigNodeHelper<decltype(clearDepth)>::serialize(clearDepth, context);
		node["mask"] = Halley::ConfigNodeHelper<decltype(mask)>::serialize(mask, context);
		node["layer"] = Halley::ConfigNodeHelper<decltype(layer)>::serialize(layer, context);
		return node;
	}

	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		Halley::ConfigNodeHelper<decltype(zoom)>::deserialize(zoom, context, node["zoom"]);
		Halley::ConfigNodeHelper<decltype(clearColour)>::deserialize(clearColour, context, node["clearColour"]);
		Halley::ConfigNodeHelper<decltype(clearStencil)>::deserialize(clearStencil, context, node["clearStencil"]);
		Halley::ConfigNodeHelper<decltype(clearDepth)>::deserialize(clearDepth, context, node["clearDepth"]);
		Halley::ConfigNodeHelper<decltype(mask)>::deserialize(mask, context, node["mask"]);
		Halley::ConfigNodeHelper<decltype(layer)>::deserialize(layer, context, node["layer"]);
	}

};
