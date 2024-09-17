// Halley codegen version 136
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


template <typename T>
class Transform2DComponentBase : public Halley::Component {
public:
	static constexpr int componentIndex{ 0 };
	static const constexpr char* componentName{ "Transform2D" };

	Transform2DComponentBase() {
	}

	Transform2DComponentBase(Halley::Vector2f position, Halley::Vector2f scale, Halley::Angle1f rotation, float height, bool fixedHeight, Halley::OptionalLite<int16_t> subWorld)
		: position(std::move(position))
		, scale(std::move(scale))
		, rotation(std::move(rotation))
		, height(std::move(height))
		, fixedHeight(std::move(fixedHeight))
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
		Halley::EntityConfigNodeSerializer<decltype(fixedHeight)>::serialize(fixedHeight, bool{ false }, _context, _node, componentName, "fixedHeight", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::serialize(subWorld, Halley::OptionalLite<int16_t>{}, _context, _node, componentName, "subWorld", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(position)>::deserialize(position, Halley::Vector2f{}, _context, _node, componentName, "position", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scale)>::deserialize(scale, Halley::Vector2f{ 1.0f, 1.0f }, _context, _node, componentName, "scale", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rotation)>::deserialize(rotation, Halley::Angle1f{}, _context, _node, componentName, "rotation", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(height)>::deserialize(height, float{}, _context, _node, componentName, "height", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(fixedHeight)>::deserialize(fixedHeight, bool{ false }, _context, _node, componentName, "fixedHeight", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(subWorld)>::deserialize(subWorld, Halley::OptionalLite<int16_t>{}, _context, _node, componentName, "subWorld", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("position");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("scale");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("rotation");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("height");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("fixedHeight");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("subWorld");
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
		if (_fieldName == "fixedHeight") {
			return Halley::ConfigNodeHelper<decltype(fixedHeight)>::serialize(fixedHeight, _context);
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
		if (_fieldName == "fixedHeight") {
			Halley::ConfigNodeHelper<decltype(fixedHeight)>::deserialize(fixedHeight, _context, _node);
			return;
		}
		if (_fieldName == "subWorld") {
			Halley::ConfigNodeHelper<decltype(subWorld)>::deserialize(subWorld, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		static_assert(std::is_base_of_v<Transform2DComponentBase, T>);
		static_assert(!std::is_same_v<Transform2DComponentBase, T>);
		return doNew<T>(size, align);
	}

	void* operator new(std::size_t size) {
		static_assert(std::is_base_of_v<Transform2DComponentBase, T>);
		static_assert(!std::is_same_v<Transform2DComponentBase, T>);
		return doNew<T>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<T>(ptr);
	}

protected:
	Halley::Vector2f position{};
	Halley::Vector2f scale{ 1.0f, 1.0f };
	Halley::Angle1f rotation{};
	float height{};
	bool fixedHeight{ false };
	Halley::OptionalLite<int16_t> subWorld{};
	mutable uint16_t revision{ 0 };

};
