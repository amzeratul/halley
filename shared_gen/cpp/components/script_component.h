#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 8 };
	static const constexpr char* componentName{ "Script" };

	Halley::ScriptGraph scriptGraph{};
	Halley::ScriptState scriptState{};

	ScriptComponent() {
	}

	ScriptComponent(Halley::ScriptGraph scriptGraph)
		: scriptGraph(std::move(scriptGraph))
	{
	}

	Halley::ConfigNode serialize(const Halley::ConfigNodeSerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(scriptGraph)>::serialize(scriptGraph, Halley::ScriptGraph{}, context, node, "scriptGraph", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(scriptState)>::serialize(scriptState, Halley::ScriptState{}, context, node, "scriptState", makeMask(Type::SaveData));
		return node;
	}

	void deserialize(const Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(scriptGraph)>::deserialize(scriptGraph, Halley::ScriptGraph{}, context, node, "scriptGraph", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(scriptState)>::deserialize(scriptState, Halley::ScriptState{}, context, node, "scriptState", makeMask(Type::SaveData));
	}

};
