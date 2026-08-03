// Halley codegen version 141
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/bytes/byte_serializer.h"
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class NetworkComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 16 };
	static constexpr uint32_t quickIndex{ 4 };
	static const constexpr char* componentName{ "Network" };
	static constexpr bool alwaysEnabled{ false };

	std::optional<uint8_t> ownerId{};
	std::optional<uint8_t> authorityId{};
	std::optional<uint8_t> creatorId{};
	bool sendUpdates{ false };
	bool alwaysSend{ false };
	bool requiresEntityFrameModified{ false };
	Halley::DataInterpolatorSet dataInterpolatorSet{};
	Halley::ByteDataInterpolatorSet byteDataInterpolatorSet{};
	Halley::Vector<std::pair<Halley::EntityId, uint8_t>> locks{};

	NetworkComponent() {
	}

	NetworkComponent(bool alwaysSend, bool requiresEntityFrameModified)
		: alwaysSend(std::move(alwaysSend))
		, requiresEntityFrameModified(std::move(requiresEntityFrameModified))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(authorityId)>::serialize(authorityId, std::optional<uint8_t>{}, _context, _node, componentName, "authorityId", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(alwaysSend)>::serialize(alwaysSend, bool{ false }, _context, _node, componentName, "alwaysSend", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(requiresEntityFrameModified)>::serialize(requiresEntityFrameModified, bool{ false }, _context, _node, componentName, "requiresEntityFrameModified", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(locks)>::serialize(locks, Halley::Vector<std::pair<Halley::EntityId, uint8_t>>{}, _context, _node, componentName, "locks", makeMask(Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(authorityId)>::deserialize(authorityId, std::optional<uint8_t>{}, _context, _node, componentName, "authorityId", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(alwaysSend)>::deserialize(alwaysSend, bool{ false }, _context, _node, componentName, "alwaysSend", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(requiresEntityFrameModified)>::deserialize(requiresEntityFrameModified, bool{ false }, _context, _node, componentName, "requiresEntityFrameModified", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(locks)>::deserialize(locks, Halley::Vector<std::pair<Halley::EntityId, uint8_t>>{}, _context, _node, componentName, "locks", makeMask(Type::Network));
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Network)) == 0) _node.removeKey("authorityId");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic)) == 0) _node.removeKey("alwaysSend");
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("requiresEntityFrameModified");
		if ((_mask & makeMask(Type::Network)) == 0) _node.removeKey("locks");
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "alwaysSend") {
			return Halley::ConfigNodeHelper<decltype(alwaysSend)>::serialize(alwaysSend, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "alwaysSend") {
			Halley::ConfigNodeHelper<decltype(alwaysSend)>::deserialize(alwaysSend, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void serializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Serializer& _serializer) const {
		Halley::ByteSerializationHelper<decltype(authorityId)>::serialize(authorityId, _context, _serializer, componentIndex, "authorityId");
		Halley::ByteSerializationHelper<decltype(locks)>::serialize(locks, _context, _serializer, componentIndex, "locks");
	}

	void deserializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Deserializer& _deserializer) {
		Halley::ByteSerializationHelper<decltype(authorityId)>::deserialize(authorityId, _context, _deserializer, componentIndex, "authorityId");
		Halley::ByteSerializationHelper<decltype(locks)>::deserialize(locks, _context, _deserializer, componentIndex, "locks");
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<NetworkComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<NetworkComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<NetworkComponent>(ptr);
	}

};
