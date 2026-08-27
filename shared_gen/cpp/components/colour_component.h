// Halley codegen version 146
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/bytes/byte_serializer.h"
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class ColourComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 3 };
	static constexpr uint32_t quickIndex{ 4 };
	static const constexpr char* componentName{ "Colour" };
	static constexpr bool alwaysEnabled{ false };

	Halley::Colour4f colour{ "#FFFFFF" };
	float intensity{ 1 };
	bool propagate{ true };
	bool addedDynamically{ false };
	bool ignoreSerializeNetwork{ false };

	ColourComponent() {
	}

	ColourComponent(Halley::Colour4f colour, float intensity, bool propagate)
		: colour(std::move(colour))
		, intensity(std::move(intensity))
		, propagate(std::move(propagate))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(colour)>::serialize(colour, Halley::Colour4f{ "#FFFFFF" }, _context, _node, componentName, "colour", makeMask(Type::Prefab, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(intensity)>::serialize(intensity, float{ 1 }, _context, _node, componentName, "intensity", makeMask(Type::Prefab, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(propagate)>::serialize(propagate, bool{ true }, _context, _node, componentName, "propagate", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(addedDynamically)>::serialize(addedDynamically, bool{ false }, _context, _node, componentName, "addedDynamically", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(colour)>::deserialize(colour, Halley::Colour4f{ "#FFFFFF" }, _context, _node, componentName, "colour", makeMask(Type::Prefab, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(intensity)>::deserialize(intensity, float{ 1 }, _context, _node, componentName, "intensity", makeMask(Type::Prefab, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(propagate)>::deserialize(propagate, bool{ true }, _context, _node, componentName, "propagate", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(addedDynamically)>::deserialize(addedDynamically, bool{ false }, _context, _node, componentName, "addedDynamically", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
	}

	void hash(const Halley::EntitySerializationContext& _context, Halley::Hash::Hasher& _hasher) const {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(colour)>::hash<makeMask(Type::Prefab, Type::Dynamic, Type::Network)>(_hasher, colour, _context, "colour");
		Halley::EntityConfigNodeSerializer<decltype(intensity)>::hash<makeMask(Type::Prefab, Type::Dynamic, Type::Network)>(_hasher, intensity, _context, "intensity");
		Halley::EntityConfigNodeSerializer<decltype(propagate)>::hash<makeMask(Type::Prefab)>(_hasher, propagate, _context, "propagate");
		Halley::EntityConfigNodeSerializer<decltype(addedDynamically)>::hash<makeMask(Type::SaveData, Type::Dynamic, Type::Network)>(_hasher, addedDynamically, _context, "addedDynamically");
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Prefab, Type::Dynamic, Type::Network)) == 0) _node.removeKey("colour");
		if ((_mask & makeMask(Type::Prefab, Type::Dynamic, Type::Network)) == 0) _node.removeKey("intensity");
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("propagate");
		if ((_mask & makeMask(Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("addedDynamically");
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "colour") {
			return Halley::ConfigNodeHelper<decltype(colour)>::serialize(colour, _context);
		}
		if (_fieldName == "intensity") {
			return Halley::ConfigNodeHelper<decltype(intensity)>::serialize(intensity, _context);
		}
		if (_fieldName == "addedDynamically") {
			return Halley::ConfigNodeHelper<decltype(addedDynamically)>::serialize(addedDynamically, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "colour") {
			Halley::ConfigNodeHelper<decltype(colour)>::deserialize(colour, _context, _node);
			return;
		}
		if (_fieldName == "intensity") {
			Halley::ConfigNodeHelper<decltype(intensity)>::deserialize(intensity, _context, _node);
			return;
		}
		if (_fieldName == "addedDynamically") {
			Halley::ConfigNodeHelper<decltype(addedDynamically)>::deserialize(addedDynamically, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void serializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Serializer& _serializer) const {
		Halley::ByteSerializationHelper<decltype(colour)>::serialize(colour, _context, _serializer, componentIndex, "colour");
		Halley::ByteSerializationHelper<decltype(intensity)>::serialize(intensity, _context, _serializer, componentIndex, "intensity");
		Halley::ByteSerializationHelper<decltype(addedDynamically)>::serialize(addedDynamically, _context, _serializer, componentIndex, "addedDynamically");
	}

	void deserializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Deserializer& _deserializer) {
		Halley::ByteSerializationHelper<decltype(colour)>::deserialize(colour, _context, _deserializer, componentIndex, "colour");
		Halley::ByteSerializationHelper<decltype(intensity)>::deserialize(intensity, _context, _deserializer, componentIndex, "intensity");
		Halley::ByteSerializationHelper<decltype(addedDynamically)>::deserialize(addedDynamically, _context, _deserializer, componentIndex, "addedDynamically");
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<ColourComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<ColourComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<ColourComponent>(ptr);
	}

};
