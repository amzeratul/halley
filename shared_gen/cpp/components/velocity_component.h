// Halley codegen version 148
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/bytes/byte_serializer.h"
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class VelocityComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 1 };
	static constexpr uint32_t quickIndex{ 0 };
	static const constexpr char* componentName{ "Velocity" };
	static constexpr bool alwaysEnabled{ false };

	Halley::Vector2f velocity{};
	bool enabled{ true };

	VelocityComponent() {
	}

	VelocityComponent(Halley::Vector2f velocity, bool enabled)
		: velocity(std::move(velocity))
		, enabled(std::move(enabled))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(velocity)>::serialize(velocity, Halley::Vector2f{}, _context, _node, componentName, "velocity", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(enabled)>::serialize(enabled, bool{ true }, _context, _node, componentName, "enabled", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(velocity)>::deserialize(velocity, Halley::Vector2f{}, _context, _node, componentName, "velocity", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(enabled)>::deserialize(enabled, bool{ true }, _context, _node, componentName, "enabled", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	void hash(const Halley::EntitySerializationContext& _context, Halley::Hash::Hasher& _hasher) const {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(velocity)>::hash(_hasher, velocity, _context, "velocity", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(enabled)>::hash(_hasher, enabled, _context, "enabled", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("velocity");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("enabled");
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "velocity") {
			return Halley::ConfigNodeHelper<decltype(velocity)>::serialize(velocity, _context);
		}
		if (_fieldName == "enabled") {
			return Halley::ConfigNodeHelper<decltype(enabled)>::serialize(enabled, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "velocity") {
			Halley::ConfigNodeHelper<decltype(velocity)>::deserialize(velocity, _context, _node);
			return;
		}
		if (_fieldName == "enabled") {
			Halley::ConfigNodeHelper<decltype(enabled)>::deserialize(enabled, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void serializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Serializer& _serializer) const {
		Halley::ByteSerializationHelper<decltype(velocity)>::serialize(velocity, _context, _serializer, componentIndex, "velocity");
		Halley::ByteSerializationHelper<decltype(enabled)>::serialize(enabled, _context, _serializer, componentIndex, "enabled");
	}

	void deserializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Deserializer& _deserializer) {
		Halley::ByteSerializationHelper<decltype(velocity)>::deserialize(velocity, _context, _deserializer, componentIndex, "velocity");
		Halley::ByteSerializationHelper<decltype(enabled)>::deserialize(enabled, _context, _deserializer, componentIndex, "enabled");
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<VelocityComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<VelocityComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<VelocityComponent>(ptr);
	}

};
