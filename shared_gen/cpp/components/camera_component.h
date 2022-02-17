#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class CameraComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 4 };
	static const constexpr char* componentName{ "Camera" };

	float zoom{ 1 };
	Halley::String id{};

	CameraComponent() {
	}

	CameraComponent(float zoom, Halley::String id)
		: zoom(std::move(zoom))
		, id(std::move(id))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(zoom)>::serialize(zoom, float{ 1 }, context, node, componentName, "zoom", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(id)>::serialize(id, Halley::String{}, context, node, componentName, "id", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(zoom)>::deserialize(zoom, float{ 1 }, context, node, componentName, "zoom", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(id)>::deserialize(id, Halley::String{}, context, node, componentName, "id", makeMask(Type::Prefab, Type::SaveData));
	}

};
