// Halley codegen version 118
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptTargetComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 10 };
	static const constexpr char* componentName{ "ScriptTarget" };

	Halley::String id{};

	ScriptTargetComponent() {
	}

	ScriptTargetComponent(Halley::String id)
		: id(std::move(id))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(id)>::serialize(id, Halley::String{}, context, node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(id)>::deserialize(id, Halley::String{}, context, node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
