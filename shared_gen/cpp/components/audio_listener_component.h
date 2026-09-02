// Halley codegen version 148
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/bytes/byte_serializer.h"
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class AudioListenerComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 10 };
	static constexpr uint32_t quickIndex{ 0 };
	static const constexpr char* componentName{ "AudioListener" };
	static constexpr bool alwaysEnabled{ false };

	float referenceDistance{ 500 };
	Halley::Vector3f lastPos{};
	Halley::RollingDataSet<Halley::Vector3f> velAverage{ 5 };
	float speedOfSound{ 343 };
	Halley::Vector<Halley::AudioListenerRegionData> regions{};

	AudioListenerComponent() {
	}

	AudioListenerComponent(float referenceDistance, float speedOfSound)
		: referenceDistance(std::move(referenceDistance))
		, speedOfSound(std::move(speedOfSound))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(referenceDistance)>::serialize(referenceDistance, float{ 500 }, _context, _node, componentName, "referenceDistance", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(lastPos)>::serialize(lastPos, Halley::Vector3f{}, _context, _node, componentName, "lastPos", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(speedOfSound)>::serialize(speedOfSound, float{ 343 }, _context, _node, componentName, "speedOfSound", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(referenceDistance)>::deserialize(referenceDistance, float{ 500 }, _context, _node, componentName, "referenceDistance", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(lastPos)>::deserialize(lastPos, Halley::Vector3f{}, _context, _node, componentName, "lastPos", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(speedOfSound)>::deserialize(speedOfSound, float{ 343 }, _context, _node, componentName, "speedOfSound", makeMask(Type::Prefab));
	}

	void hash(const Halley::EntitySerializationContext& _context, Halley::Hash::Hasher& _hasher) const {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(referenceDistance)>::hash(_hasher, referenceDistance, _context, "referenceDistance", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(lastPos)>::hash(_hasher, lastPos, _context, "lastPos", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("referenceDistance");
		if ((_mask & makeMask(Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("lastPos");
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("speedOfSound");
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "referenceDistance") {
			return Halley::ConfigNodeHelper<decltype(referenceDistance)>::serialize(referenceDistance, _context);
		}
		if (_fieldName == "lastPos") {
			return Halley::ConfigNodeHelper<decltype(lastPos)>::serialize(lastPos, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "referenceDistance") {
			Halley::ConfigNodeHelper<decltype(referenceDistance)>::deserialize(referenceDistance, _context, _node);
			return;
		}
		if (_fieldName == "lastPos") {
			Halley::ConfigNodeHelper<decltype(lastPos)>::deserialize(lastPos, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void serializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Serializer& _serializer) const {
		Halley::ByteSerializationHelper<decltype(referenceDistance)>::serialize(referenceDistance, _context, _serializer, componentIndex, "referenceDistance");
		Halley::ByteSerializationHelper<decltype(lastPos)>::serialize(lastPos, _context, _serializer, componentIndex, "lastPos");
	}

	void deserializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Deserializer& _deserializer) {
		Halley::ByteSerializationHelper<decltype(referenceDistance)>::deserialize(referenceDistance, _context, _deserializer, componentIndex, "referenceDistance");
		Halley::ByteSerializationHelper<decltype(lastPos)>::deserialize(lastPos, _context, _deserializer, componentIndex, "lastPos");
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<AudioListenerComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<AudioListenerComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<AudioListenerComponent>(ptr);
	}

};
