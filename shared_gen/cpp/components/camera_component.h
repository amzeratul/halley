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
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(zoom)>::serialize(zoom, context, node, "zoom", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(clearColour)>::serialize(clearColour, context, node, "clearColour", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(clearStencil)>::serialize(clearStencil, context, node, "clearStencil", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(clearDepth)>::serialize(clearDepth, context, node, "clearDepth", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, context, node, "mask", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, context, node, "layer", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(zoom)>::deserialize(zoom, context, node, "zoom", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(clearColour)>::deserialize(clearColour, context, node, "clearColour", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(clearStencil)>::deserialize(clearStencil, context, node, "clearStencil", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(clearDepth)>::deserialize(clearDepth, context, node, "clearDepth", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, context, node, "mask", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, context, node, "layer", makeMask(Type::Prefab, Type::SaveData));
	}

};
