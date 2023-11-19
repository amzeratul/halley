// Halley codegen version 127
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif
#include "halley/support/exception.h"


class SpriteComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 2 };
	static const constexpr char* componentName{ "Sprite" };

	Halley::Sprite sprite{};
	int layer{ 0 };
	Halley::OptionalLite<int> mask{};

	SpriteComponent() {
	}

	SpriteComponent(Halley::Sprite sprite, int layer, Halley::OptionalLite<int> mask)
		: sprite(std::move(sprite))
		, layer(std::move(layer))
		, mask(std::move(mask))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(sprite)>::serialize(sprite, Halley::Sprite{}, _context, _node, componentName, "sprite", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, Halley::OptionalLite<int>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(sprite)>::deserialize(sprite, Halley::Sprite{}, _context, _node, componentName, "sprite", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, int{ 0 }, _context, _node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, Halley::OptionalLite<int>{}, _context, _node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "sprite") {
			return Halley::ConfigNodeHelper<decltype(sprite)>::serialize(sprite, _context);
		}
		if (_fieldName == "layer") {
			return Halley::ConfigNodeHelper<decltype(layer)>::serialize(layer, _context);
		}
		if (_fieldName == "mask") {
			return Halley::ConfigNodeHelper<decltype(mask)>::serialize(mask, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "sprite") {
			Halley::ConfigNodeHelper<decltype(sprite)>::deserialize(sprite, _context, _node);
			return;
		}
		if (_fieldName == "layer") {
			Halley::ConfigNodeHelper<decltype(layer)>::deserialize(layer, _context, _node);
			return;
		}
		if (_fieldName == "mask") {
			Halley::ConfigNodeHelper<decltype(mask)>::deserialize(mask, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<SpriteComponent>(size, align);
	}

	void operator delete(void* ptr) {
		return doDelete<SpriteComponent>(ptr);
	}

};
