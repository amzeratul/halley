// Halley codegen version 101
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class AudioListenerComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 6 };
	static const constexpr char* componentName{ "AudioListener" };

	float referenceDistance{ 500 };

	AudioListenerComponent() {
	}

	AudioListenerComponent(float referenceDistance)
		: referenceDistance(std::move(referenceDistance))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(referenceDistance)>::serialize(referenceDistance, float{ 500 }, context, node, componentName, "referenceDistance", makeMask(Type::Prefab, Type::SaveData));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(referenceDistance)>::deserialize(referenceDistance, float{ 500 }, context, node, componentName, "referenceDistance", makeMask(Type::Prefab, Type::SaveData));
	}

};
