// Halley codegen version 125
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif
#include "halley/support/exception.h"


class ColourComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 3 };
	static const constexpr char* componentName{ "Colour" };

	Halley::Colour4f colour{ "#FFFFFF" };
	float intensity{ 1 };

	ColourComponent() {
	}

	ColourComponent(Halley::Colour4f colour, float intensity)
		: colour(std::move(colour))
		, intensity(std::move(intensity))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(colour)>::serialize(colour, Halley::Colour4f{ "#FFFFFF" }, _context, _node, componentName, "colour", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(intensity)>::serialize(intensity, float{ 1 }, _context, _node, componentName, "intensity", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(colour)>::deserialize(colour, Halley::Colour4f{ "#FFFFFF" }, _context, _node, componentName, "colour", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(intensity)>::deserialize(intensity, float{ 1 }, _context, _node, componentName, "intensity", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "colour") {
			return Halley::ConfigNodeHelper<decltype(colour)>::serialize(colour, _context);
		}
		if (_fieldName == "intensity") {
			return Halley::ConfigNodeHelper<decltype(intensity)>::serialize(intensity, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "colour") {
			Halley::ConfigNodeHelper<decltype(colour)>::deserialize(colour, _context, _node);
			return;
		}
		if (_fieldName == "intensity") {
			Halley::ConfigNodeHelper<decltype(intensity)>::deserialize(intensity, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

};
