// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptTargetComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 13 };
	static const constexpr char* componentName{ "ScriptTarget" };

	Halley::String id{};

	ScriptTargetComponent() {
	}

	ScriptTargetComponent(Halley::String id)
		: id(std::move(id))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(id)>::serialize(id, Halley::String{}, _context, _node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(id)>::deserialize(id, Halley::String{}, _context, _node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
