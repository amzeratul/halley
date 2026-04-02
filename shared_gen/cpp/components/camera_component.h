// Halley codegen version 140
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/bytes/byte_serializer.h"
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class CameraComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 6 };
	static const constexpr char* componentName{ "Camera" };
	static constexpr bool alwaysEnabled{ false };

	float zoom{ 1 };
	Halley::String id{};
	Halley::HashMap<Halley::String, Halley::Vector2f> uiOffsets{};
	Halley::HashMap<Halley::String, Halley::Vector2f> worldOffsets{};
	bool integerCoords{ false };

	CameraComponent() {
	}

	CameraComponent(float zoom, Halley::String id, Halley::HashMap<Halley::String, Halley::Vector2f> uiOffsets, Halley::HashMap<Halley::String, Halley::Vector2f> worldOffsets, bool integerCoords)
		: zoom(std::move(zoom))
		, id(std::move(id))
		, uiOffsets(std::move(uiOffsets))
		, worldOffsets(std::move(worldOffsets))
		, integerCoords(std::move(integerCoords))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(zoom)>::serialize(zoom, float{ 1 }, _context, _node, componentName, "zoom", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(id)>::serialize(id, Halley::String{}, _context, _node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(uiOffsets)>::serialize(uiOffsets, Halley::HashMap<Halley::String, Halley::Vector2f>{}, _context, _node, componentName, "uiOffsets", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(worldOffsets)>::serialize(worldOffsets, Halley::HashMap<Halley::String, Halley::Vector2f>{}, _context, _node, componentName, "worldOffsets", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(integerCoords)>::serialize(integerCoords, bool{ false }, _context, _node, componentName, "integerCoords", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(zoom)>::deserialize(zoom, float{ 1 }, _context, _node, componentName, "zoom", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(id)>::deserialize(id, Halley::String{}, _context, _node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(uiOffsets)>::deserialize(uiOffsets, Halley::HashMap<Halley::String, Halley::Vector2f>{}, _context, _node, componentName, "uiOffsets", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(worldOffsets)>::deserialize(worldOffsets, Halley::HashMap<Halley::String, Halley::Vector2f>{}, _context, _node, componentName, "worldOffsets", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(integerCoords)>::deserialize(integerCoords, bool{ false }, _context, _node, componentName, "integerCoords", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("zoom");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("id");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("uiOffsets");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("worldOffsets");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("integerCoords");
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "zoom") {
			return Halley::ConfigNodeHelper<decltype(zoom)>::serialize(zoom, _context);
		}
		if (_fieldName == "id") {
			return Halley::ConfigNodeHelper<decltype(id)>::serialize(id, _context);
		}
		if (_fieldName == "uiOffsets") {
			return Halley::ConfigNodeHelper<decltype(uiOffsets)>::serialize(uiOffsets, _context);
		}
		if (_fieldName == "worldOffsets") {
			return Halley::ConfigNodeHelper<decltype(worldOffsets)>::serialize(worldOffsets, _context);
		}
		if (_fieldName == "integerCoords") {
			return Halley::ConfigNodeHelper<decltype(integerCoords)>::serialize(integerCoords, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "zoom") {
			Halley::ConfigNodeHelper<decltype(zoom)>::deserialize(zoom, _context, _node);
			return;
		}
		if (_fieldName == "id") {
			Halley::ConfigNodeHelper<decltype(id)>::deserialize(id, _context, _node);
			return;
		}
		if (_fieldName == "uiOffsets") {
			Halley::ConfigNodeHelper<decltype(uiOffsets)>::deserialize(uiOffsets, _context, _node);
			return;
		}
		if (_fieldName == "worldOffsets") {
			Halley::ConfigNodeHelper<decltype(worldOffsets)>::deserialize(worldOffsets, _context, _node);
			return;
		}
		if (_fieldName == "integerCoords") {
			Halley::ConfigNodeHelper<decltype(integerCoords)>::deserialize(integerCoords, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void serializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Serializer& _serializer) const {
		Halley::ByteSerializationHelper<decltype(zoom)>::serialize(zoom, _context, _serializer, componentIndex, "zoom");
		Halley::ByteSerializationHelper<decltype(id)>::serialize(id, _context, _serializer, componentIndex, "id");
		Halley::ByteSerializationHelper<decltype(uiOffsets)>::serialize(uiOffsets, _context, _serializer, componentIndex, "uiOffsets");
		Halley::ByteSerializationHelper<decltype(worldOffsets)>::serialize(worldOffsets, _context, _serializer, componentIndex, "worldOffsets");
		Halley::ByteSerializationHelper<decltype(integerCoords)>::serialize(integerCoords, _context, _serializer, componentIndex, "integerCoords");
	}

	void deserializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Deserializer& _deserializer) {
		Halley::ByteSerializationHelper<decltype(zoom)>::deserialize(zoom, _context, _deserializer, componentIndex, "zoom");
		Halley::ByteSerializationHelper<decltype(id)>::deserialize(id, _context, _deserializer, componentIndex, "id");
		Halley::ByteSerializationHelper<decltype(uiOffsets)>::deserialize(uiOffsets, _context, _deserializer, componentIndex, "uiOffsets");
		Halley::ByteSerializationHelper<decltype(worldOffsets)>::deserialize(worldOffsets, _context, _deserializer, componentIndex, "worldOffsets");
		Halley::ByteSerializationHelper<decltype(integerCoords)>::deserialize(integerCoords, _context, _deserializer, componentIndex, "integerCoords");
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<CameraComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<CameraComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<CameraComponent>(ptr);
	}

};
