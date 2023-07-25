// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ColourComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 3 };
	static const constexpr char* componentName{ "Colour" };

	Halley::Colour4f colour{ "#FFFFFF" };

	ColourComponent() {
	}

	ColourComponent(Halley::Colour4f colour)
		: colour(std::move(colour))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(colour)>::serialize(colour, Halley::Colour4f{ "#FFFFFF" }, _context, _node, componentName, "colour", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(colour)>::deserialize(colour, Halley::Colour4f{ "#FFFFFF" }, _context, _node, componentName, "colour", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
