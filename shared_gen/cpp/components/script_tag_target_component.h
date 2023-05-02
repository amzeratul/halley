// Halley codegen version 120
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptTagTargetComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 11 };
	static const constexpr char* componentName{ "ScriptTagTarget" };

	Halley::Vector<Halley::String> tags{};

	ScriptTagTargetComponent() {
	}

	ScriptTagTargetComponent(Halley::Vector<Halley::String> tags)
		: tags(std::move(tags))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(tags)>::serialize(tags, Halley::Vector<Halley::String>{}, context, node, componentName, "tags", makeMask(Type::Prefab));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(tags)>::deserialize(tags, Halley::Vector<Halley::String>{}, context, node, componentName, "tags", makeMask(Type::Prefab));
	}

};
