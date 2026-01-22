// Halley codegen version 138
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/bytes/byte_serializer.h"
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class AudioSourceComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 11 };
	static const constexpr char* componentName{ "AudioSource" };

	Halley::AudioEmitterHandle emitter{};
	Halley::ResourceReference<Halley::AudioEvent> event{};
	float rangeMin{ 50 };
	float rangeMax{ 100 };
	float rollOff{ 1 };
	Halley::AudioAttenuationCurve curve{ Halley::AudioAttenuationCurve::Linear };
	Halley::Polygon polygon{};
	Halley::Vector3f lastPos{};
	bool canAutoVel{ false };
	bool moved{ false };
	Halley::HashMap<Halley::String, Halley::ResourceReference<Halley::AudioEvent>> dynamicEvents{};
	Halley::Vector2f offset{};

	AudioSourceComponent() {
	}

	AudioSourceComponent(Halley::ResourceReference<Halley::AudioEvent> event, float rangeMin, float rangeMax, float rollOff, Halley::AudioAttenuationCurve curve, Halley::Polygon polygon, bool canAutoVel, Halley::HashMap<Halley::String, Halley::ResourceReference<Halley::AudioEvent>> dynamicEvents, Halley::Vector2f offset)
		: event(std::move(event))
		, rangeMin(std::move(rangeMin))
		, rangeMax(std::move(rangeMax))
		, rollOff(std::move(rollOff))
		, curve(std::move(curve))
		, polygon(std::move(polygon))
		, canAutoVel(std::move(canAutoVel))
		, dynamicEvents(std::move(dynamicEvents))
		, offset(std::move(offset))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(event)>::serialize(event, Halley::ResourceReference<Halley::AudioEvent>{}, _context, _node, componentName, "event", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMin)>::serialize(rangeMin, float{ 50 }, _context, _node, componentName, "rangeMin", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMax)>::serialize(rangeMax, float{ 100 }, _context, _node, componentName, "rangeMax", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rollOff)>::serialize(rollOff, float{ 1 }, _context, _node, componentName, "rollOff", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(curve)>::serialize(curve, Halley::AudioAttenuationCurve{ Halley::AudioAttenuationCurve::Linear }, _context, _node, componentName, "curve", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(polygon)>::serialize(polygon, Halley::Polygon{}, _context, _node, componentName, "polygon", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(canAutoVel)>::serialize(canAutoVel, bool{ false }, _context, _node, componentName, "canAutoVel", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(dynamicEvents)>::serialize(dynamicEvents, Halley::HashMap<Halley::String, Halley::ResourceReference<Halley::AudioEvent>>{}, _context, _node, componentName, "dynamicEvents", makeMask(Type::Prefab, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(offset)>::serialize(offset, Halley::Vector2f{}, _context, _node, componentName, "offset", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(event)>::deserialize(event, Halley::ResourceReference<Halley::AudioEvent>{}, _context, _node, componentName, "event", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMin)>::deserialize(rangeMin, float{ 50 }, _context, _node, componentName, "rangeMin", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMax)>::deserialize(rangeMax, float{ 100 }, _context, _node, componentName, "rangeMax", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rollOff)>::deserialize(rollOff, float{ 1 }, _context, _node, componentName, "rollOff", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(curve)>::deserialize(curve, Halley::AudioAttenuationCurve{ Halley::AudioAttenuationCurve::Linear }, _context, _node, componentName, "curve", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(polygon)>::deserialize(polygon, Halley::Polygon{}, _context, _node, componentName, "polygon", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(canAutoVel)>::deserialize(canAutoVel, bool{ false }, _context, _node, componentName, "canAutoVel", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(dynamicEvents)>::deserialize(dynamicEvents, Halley::HashMap<Halley::String, Halley::ResourceReference<Halley::AudioEvent>>{}, _context, _node, componentName, "dynamicEvents", makeMask(Type::Prefab, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(offset)>::deserialize(offset, Halley::Vector2f{}, _context, _node, componentName, "offset", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("event");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("rangeMin");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("rangeMax");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("rollOff");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("curve");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("polygon");
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("canAutoVel");
		if ((_mask & makeMask(Type::Prefab, Type::Dynamic)) == 0) _node.removeKey("dynamicEvents");
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network)) == 0) _node.removeKey("offset");
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "event") {
			return Halley::ConfigNodeHelper<decltype(event)>::serialize(event, _context);
		}
		if (_fieldName == "rangeMin") {
			return Halley::ConfigNodeHelper<decltype(rangeMin)>::serialize(rangeMin, _context);
		}
		if (_fieldName == "rangeMax") {
			return Halley::ConfigNodeHelper<decltype(rangeMax)>::serialize(rangeMax, _context);
		}
		if (_fieldName == "rollOff") {
			return Halley::ConfigNodeHelper<decltype(rollOff)>::serialize(rollOff, _context);
		}
		if (_fieldName == "curve") {
			return Halley::ConfigNodeHelper<decltype(curve)>::serialize(curve, _context);
		}
		if (_fieldName == "polygon") {
			return Halley::ConfigNodeHelper<decltype(polygon)>::serialize(polygon, _context);
		}
		if (_fieldName == "dynamicEvents") {
			return Halley::ConfigNodeHelper<decltype(dynamicEvents)>::serialize(dynamicEvents, _context);
		}
		if (_fieldName == "offset") {
			return Halley::ConfigNodeHelper<decltype(offset)>::serialize(offset, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "event") {
			Halley::ConfigNodeHelper<decltype(event)>::deserialize(event, _context, _node);
			return;
		}
		if (_fieldName == "rangeMin") {
			Halley::ConfigNodeHelper<decltype(rangeMin)>::deserialize(rangeMin, _context, _node);
			return;
		}
		if (_fieldName == "rangeMax") {
			Halley::ConfigNodeHelper<decltype(rangeMax)>::deserialize(rangeMax, _context, _node);
			return;
		}
		if (_fieldName == "rollOff") {
			Halley::ConfigNodeHelper<decltype(rollOff)>::deserialize(rollOff, _context, _node);
			return;
		}
		if (_fieldName == "curve") {
			Halley::ConfigNodeHelper<decltype(curve)>::deserialize(curve, _context, _node);
			return;
		}
		if (_fieldName == "polygon") {
			Halley::ConfigNodeHelper<decltype(polygon)>::deserialize(polygon, _context, _node);
			return;
		}
		if (_fieldName == "dynamicEvents") {
			Halley::ConfigNodeHelper<decltype(dynamicEvents)>::deserialize(dynamicEvents, _context, _node);
			return;
		}
		if (_fieldName == "offset") {
			Halley::ConfigNodeHelper<decltype(offset)>::deserialize(offset, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void serializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Serializer& _serializer) const {
		Halley::ByteSerializationHelper<decltype(event)>::serialize(event, _context, _serializer, componentIndex, "event");
		Halley::ByteSerializationHelper<decltype(rangeMin)>::serialize(rangeMin, _context, _serializer, componentIndex, "rangeMin");
		Halley::ByteSerializationHelper<decltype(rangeMax)>::serialize(rangeMax, _context, _serializer, componentIndex, "rangeMax");
		Halley::ByteSerializationHelper<decltype(rollOff)>::serialize(rollOff, _context, _serializer, componentIndex, "rollOff");
		Halley::ByteSerializationHelper<decltype(curve)>::serialize(curve, _context, _serializer, componentIndex, "curve");
		Halley::ByteSerializationHelper<decltype(polygon)>::serialize(polygon, _context, _serializer, componentIndex, "polygon");
		Halley::ByteSerializationHelper<decltype(offset)>::serialize(offset, _context, _serializer, componentIndex, "offset");
	}

	void deserializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Deserializer& _deserializer) {
		Halley::ByteSerializationHelper<decltype(event)>::deserialize(event, _context, _deserializer, componentIndex, "event");
		Halley::ByteSerializationHelper<decltype(rangeMin)>::deserialize(rangeMin, _context, _deserializer, componentIndex, "rangeMin");
		Halley::ByteSerializationHelper<decltype(rangeMax)>::deserialize(rangeMax, _context, _deserializer, componentIndex, "rangeMax");
		Halley::ByteSerializationHelper<decltype(rollOff)>::deserialize(rollOff, _context, _deserializer, componentIndex, "rollOff");
		Halley::ByteSerializationHelper<decltype(curve)>::deserialize(curve, _context, _deserializer, componentIndex, "curve");
		Halley::ByteSerializationHelper<decltype(polygon)>::deserialize(polygon, _context, _deserializer, componentIndex, "polygon");
		Halley::ByteSerializationHelper<decltype(offset)>::deserialize(offset, _context, _deserializer, componentIndex, "offset");
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<AudioSourceComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<AudioSourceComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<AudioSourceComponent>(ptr);
	}

};
