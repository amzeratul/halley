// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class AudioSourceComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 9 };
	static const constexpr char* componentName{ "AudioSource" };

	Halley::AudioEmitterHandle emitter{};
	Halley::ResourceReference<Halley::AudioEvent> event{};
	float rangeMin{ 50 };
	float rangeMax{ 100 };
	Halley::Vector3f lastPos{};

	AudioSourceComponent() {
	}

	AudioSourceComponent(Halley::ResourceReference<Halley::AudioEvent> event, float rangeMin, float rangeMax)
		: event(std::move(event))
		, rangeMin(std::move(rangeMin))
		, rangeMax(std::move(rangeMax))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(event)>::serialize(event, Halley::ResourceReference<Halley::AudioEvent>{}, _context, _node, componentName, "event", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMin)>::serialize(rangeMin, float{ 50 }, _context, _node, componentName, "rangeMin", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMax)>::serialize(rangeMax, float{ 100 }, _context, _node, componentName, "rangeMax", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(event)>::deserialize(event, Halley::ResourceReference<Halley::AudioEvent>{}, _context, _node, componentName, "event", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMin)>::deserialize(rangeMin, float{ 50 }, _context, _node, componentName, "rangeMin", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(rangeMax)>::deserialize(rangeMax, float{ 100 }, _context, _node, componentName, "rangeMax", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
