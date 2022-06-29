// Halley codegen version 115
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptableComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 8 };
	static const constexpr char* componentName{ "Scriptable" };

	Halley::Vector<std::shared_ptr<Halley::ScriptState>> activeStates{};
	Halley::Vector<Halley::String> tags{};
	Halley::ScriptVariables variables{};

	ScriptableComponent() {
	}

	ScriptableComponent(Halley::Vector<Halley::String> tags)
		: tags(std::move(tags))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::serialize(activeStates, Halley::Vector<std::shared_ptr<Halley::ScriptState>>{}, context, node, componentName, "activeStates", makeMask(Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::serialize(tags, Halley::Vector<Halley::String>{}, context, node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(variables)>::serialize(variables, Halley::ScriptVariables{}, context, node, componentName, "variables", makeMask(Type::SaveData, Type::Network));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::deserialize(activeStates, Halley::Vector<std::shared_ptr<Halley::ScriptState>>{}, context, node, componentName, "activeStates", makeMask(Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::deserialize(tags, Halley::Vector<Halley::String>{}, context, node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(variables)>::deserialize(variables, Halley::ScriptVariables{}, context, node, componentName, "variables", makeMask(Type::SaveData, Type::Network));
	}

};
