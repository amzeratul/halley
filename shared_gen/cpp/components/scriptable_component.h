// Halley codegen version 105
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptableComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 8 };
	static const constexpr char* componentName{ "Scriptable" };

	Halley::ScriptState scriptState{};

	ScriptableComponent() {
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(scriptState)>::serialize(scriptState, Halley::ScriptState{}, context, node, componentName, "scriptState", makeMask(Type::SaveData, Type::Network));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(scriptState)>::deserialize(scriptState, Halley::ScriptState{}, context, node, componentName, "scriptState", makeMask(Type::SaveData, Type::Network));
	}

};
