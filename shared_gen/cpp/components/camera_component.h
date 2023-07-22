// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class CameraComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 5 };
	static const constexpr char* componentName{ "Camera" };

	float zoom{ 1 };
	Halley::String id{};
	Halley::Vector2f offset{};

	CameraComponent() {
	}

	CameraComponent(float zoom, Halley::String id, Halley::Vector2f offset)
		: zoom(std::move(zoom))
		, id(std::move(id))
		, offset(std::move(offset))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(zoom)>::serialize(zoom, float{ 1 }, _context, _node, componentName, "zoom", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(id)>::serialize(id, Halley::String{}, _context, _node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(offset)>::serialize(offset, Halley::Vector2f{}, _context, _node, componentName, "offset", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(zoom)>::deserialize(zoom, float{ 1 }, _context, _node, componentName, "zoom", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(id)>::deserialize(id, Halley::String{}, _context, _node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(offset)>::deserialize(offset, Halley::Vector2f{}, _context, _node, componentName, "offset", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
