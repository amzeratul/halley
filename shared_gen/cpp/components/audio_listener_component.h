// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class AudioListenerComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 8 };
	static const constexpr char* componentName{ "AudioListener" };

	float referenceDistance{ 500 };
	Halley::Vector3f lastPos{};
	Halley::RollingDataSet<Halley::Vector3f> velAverage{ 5 };
	float speedOfSound{ 343 };

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
		Halley::EntityConfigNodeSerializer<decltype(referenceDistance)>::serialize(referenceDistance, float{ 500 }, _context, _node, componentName, "referenceDistance", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(lastPos)>::serialize(lastPos, Halley::Vector3f{}, _context, _node, componentName, "lastPos", makeMask(Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(speedOfSound)>::serialize(speedOfSound, float{ 343 }, _context, _node, componentName, "speedOfSound", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(referenceDistance)>::deserialize(referenceDistance, float{ 500 }, _context, _node, componentName, "referenceDistance", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(lastPos)>::deserialize(lastPos, Halley::Vector3f{}, _context, _node, componentName, "lastPos", makeMask(Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(speedOfSound)>::deserialize(speedOfSound, float{ 343 }, _context, _node, componentName, "speedOfSound", makeMask(Type::Prefab));
	}

};
