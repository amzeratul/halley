// Halley codegen version 129
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class AudioSourceComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 10 };
	static const constexpr char* componentName{ "AudioSource" };

	Halley::AudioEmitterHandle emitter{};
	Halley::ResourceReference<Halley::AudioEvent> event{};
	float rangeMin{ 50 };
	float rangeMax{ 100 };
	float rollOff{ 1 };
	Halley::AudioAttenuationCurve curve{ Halley::AudioAttenuationCurve::Linear };
	Halley::Vector3f lastPos{};
	bool canAutoVel{ false };
	bool moved{ false };

	AudioSourceComponent() {
	}

	AudioSourceComponent(Halley::ResourceReference<Halley::AudioEvent> event, float rangeMin, float rangeMax, float rollOff, Halley::AudioAttenuationCurve curve, bool canAutoVel)
		: event(std::move(event))
		, rangeMin(std::move(rangeMin))
		, rangeMax(std::move(rangeMax))
		, rollOff(std::move(rollOff))
		, curve(std::move(curve))
		, canAutoVel(std::move(canAutoVel))
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
		Halley::EntityConfigNodeSerializer<decltype(canAutoVel)>::serialize(canAutoVel, bool{ false }, _context, _node, componentName, "canAutoVel", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(event)>::deserialize(event, Halley::ResourceReference<Halley::AudioEvent>{}, _context, _node, componentName, "event", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMin)>::deserialize(rangeMin, float{ 50 }, _context, _node, componentName, "rangeMin", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMax)>::deserialize(rangeMax, float{ 100 }, _context, _node, componentName, "rangeMax", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rollOff)>::deserialize(rollOff, float{ 1 }, _context, _node, componentName, "rollOff", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(curve)>::deserialize(curve, Halley::AudioAttenuationCurve{ Halley::AudioAttenuationCurve::Linear }, _context, _node, componentName, "curve", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(canAutoVel)>::deserialize(canAutoVel, bool{ false }, _context, _node, componentName, "canAutoVel", makeMask(Type::Prefab));
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
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
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
