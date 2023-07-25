// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptTagTargetComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 14 };
	static const constexpr char* componentName{ "ScriptTagTarget" };

	Halley::Vector<Halley::String> tags{};

	ScriptTagTargetComponent() {
	}

	ScriptTagTargetComponent(Halley::Vector<Halley::String> tags)
		: tags(std::move(tags))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(tags)>::serialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(tags)>::deserialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab));
	}

};
