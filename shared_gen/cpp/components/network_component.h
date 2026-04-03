// Halley codegen version 140
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
	static const constexpr char* componentName{ "Network" };
	static constexpr bool alwaysEnabled{ false };

	std::optional<uint8_t> ownerId{};
	std::optional<uint8_t> authorityId{};
	Halley::DataInterpolatorSet dataInterpolatorSet{};
	Halley::ByteDataInterpolatorSet byteDataInterpolatorSet{};
	Halley::Vector<std::pair<Halley::EntityId, uint8_t>> locks{};
	bool sendUpdates{ false };
	std::optional<uint8_t> creatorId{};

	NetworkComponent() {
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(sendUpdates)>::serialize(sendUpdates, bool{ false }, _context, _node, componentName, "sendUpdates", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(sendUpdates)>::deserialize(sendUpdates, bool{ false }, _context, _node, componentName, "sendUpdates", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("sendUpdates");
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "sendUpdates") {
			return Halley::ConfigNodeHelper<decltype(sendUpdates)>::serialize(sendUpdates, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "sendUpdates") {
			Halley::ConfigNodeHelper<decltype(sendUpdates)>::deserialize(sendUpdates, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void serializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Serializer& _serializer) const {
		Halley::ByteSerializationHelper<decltype(sendUpdates)>::serialize(sendUpdates, _context, _serializer, componentIndex, "sendUpdates");
	}

	void deserializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Deserializer& _deserializer) {
		Halley::ByteSerializationHelper<decltype(sendUpdates)>::deserialize(sendUpdates, _context, _deserializer, componentIndex, "sendUpdates");
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
