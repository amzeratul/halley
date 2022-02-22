// Halley codegen version 101
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class AudioSourceComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 7 };
	static const constexpr char* componentName{ "AudioSource" };

	Halley::ResourceReference<Halley::AudioEvent> event{};
	float rangeMin{ 50 };
	float rangeMax{ 100 };
	Halley::Vector<Halley::AudioHandle> playing{};

	AudioSourceComponent() {
	}

	AudioSourceComponent(Halley::ResourceReference<Halley::AudioEvent> event, float rangeMin, float rangeMax)
		: event(std::move(event))
		, rangeMin(std::move(rangeMin))
		, rangeMax(std::move(rangeMax))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(event)>::serialize(event, Halley::ResourceReference<Halley::AudioEvent>{}, context, node, componentName, "event", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(rangeMin)>::serialize(rangeMin, float{ 50 }, context, node, componentName, "rangeMin", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(rangeMax)>::serialize(rangeMax, float{ 100 }, context, node, componentName, "rangeMax", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(event)>::deserialize(event, Halley::ResourceReference<Halley::AudioEvent>{}, context, node, componentName, "event", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(rangeMin)>::deserialize(rangeMin, float{ 50 }, context, node, componentName, "rangeMin", makeMask(Type::Prefab, Type::SaveData));
		Halley::EntityConfigNodeSerializer<decltype(rangeMax)>::deserialize(rangeMax, float{ 100 }, context, node, componentName, "rangeMax", makeMask(Type::Prefab, Type::SaveData));
	}

};
