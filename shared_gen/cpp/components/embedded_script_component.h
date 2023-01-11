// Halley codegen version 118
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class EmbeddedScriptComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 9 };
	static const constexpr char* componentName{ "EmbeddedScript" };

	Halley::ScriptGraph script{};

	EmbeddedScriptComponent() {
	}

	EmbeddedScriptComponent(Halley::ScriptGraph script)
		: script(std::move(script))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(script)>::serialize(script, Halley::ScriptGraph{}, context, node, componentName, "script", makeMask(Type::Prefab));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(script)>::deserialize(script, Halley::ScriptGraph{}, context, node, componentName, "script", makeMask(Type::Prefab));
	}

};
