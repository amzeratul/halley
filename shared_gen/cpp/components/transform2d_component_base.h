// Halley codegen version 125
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif
#include "halley/support/exception.h"


class Transform2DComponentBase : public Halley::Component {
public:
	static constexpr int componentIndex{ 0 };
	static const constexpr char* componentName{ "Transform2D" };

	Transform2DComponentBase() {
	}

	Transform2DComponentBase(Halley::Vector2f position, Halley::Vector2f scale, Halley::Angle1f rotation, float height, Halley::OptionalLite<int16_t> subWorld)
		: position(std::move(position))
		, scale(std::move(scale))
		, rotation(std::move(rotation))
		, height(std::move(height))
		, subWorld(std::move(subWorld))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(position)>::serialize(position, Halley::Vector2f{}, _context, _node, componentName, "position", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scale)>::serialize(scale, Halley::Vector2f{ 1.0f, 1.0f }, _context, _node, componentName, "scale", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rotation)>::serialize(rotation, Halley::Angle1f{}, _context, _node, componentName, "rotation", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(height)>::serialize(height, float{}, _context, _node, componentName, "height", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::serialize(subWorld, Halley::OptionalLite<int16_t>{}, _context, _node, componentName, "subWorld", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(position)>::deserialize(position, Halley::Vector2f{}, _context, _node, componentName, "position", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scale)>::deserialize(scale, Halley::Vector2f{ 1.0f, 1.0f }, _context, _node, componentName, "scale", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rotation)>::deserialize(rotation, Halley::Angle1f{}, _context, _node, componentName, "rotation", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(height)>::deserialize(height, float{}, _context, _node, componentName, "height", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::deserialize(subWorld, Halley::OptionalLite<int16_t>{}, _context, _node, componentName, "subWorld", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "position") {
			return Halley::ConfigNodeHelper<decltype(position)>::serialize(position, _context);
		}
		if (_fieldName == "scale") {
			return Halley::ConfigNodeHelper<decltype(scale)>::serialize(scale, _context);
		}
		if (_fieldName == "rotation") {
			return Halley::ConfigNodeHelper<decltype(rotation)>::serialize(rotation, _context);
		}
		if (_fieldName == "height") {
			return Halley::ConfigNodeHelper<decltype(height)>::serialize(height, _context);
		}
		if (_fieldName == "subWorld") {
			return Halley::ConfigNodeHelper<decltype(subWorld)>::serialize(subWorld, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "position") {
			Halley::ConfigNodeHelper<decltype(position)>::deserialize(position, _context, _node);
			return;
		}
		if (_fieldName == "scale") {
			Halley::ConfigNodeHelper<decltype(scale)>::deserialize(scale, _context, _node);
			return;
		}
		if (_fieldName == "rotation") {
			Halley::ConfigNodeHelper<decltype(rotation)>::deserialize(rotation, _context, _node);
			return;
		}
		if (_fieldName == "height") {
			Halley::ConfigNodeHelper<decltype(height)>::deserialize(height, _context, _node);
			return;
		}
		if (_fieldName == "subWorld") {
			Halley::ConfigNodeHelper<decltype(subWorld)>::deserialize(subWorld, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

protected:
	Halley::Vector2f position{};
	Halley::Vector2f scale{ 1.0f, 1.0f };
	Halley::Angle1f rotation{};
	float height{};
	Halley::OptionalLite<int16_t> subWorld{};
	mutable uint16_t revision{ 0 };

};
