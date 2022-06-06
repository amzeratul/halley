// Halley codegen version 105
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptableComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 8 };
	static const constexpr char* componentName{ "Scriptable" };

	Halley::Vector<Halley::ScriptState> activeStates{};
	Halley::Vector<Halley::String> tags{};

	ScriptableComponent() {
	}

	ScriptableComponent(Halley::Vector<Halley::String> tags)
		: tags(std::move(tags))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::serialize(activeStates, Halley::Vector<Halley::ScriptState>{}, context, node, componentName, "activeStates", makeMask(Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::serialize(tags, Halley::Vector<Halley::String>{}, context, node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::deserialize(activeStates, Halley::Vector<Halley::ScriptState>{}, context, node, componentName, "activeStates", makeMask(Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::deserialize(tags, Halley::Vector<Halley::String>{}, context, node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
