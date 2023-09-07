// Halley codegen version 124
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptableComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 11 };
	static const constexpr char* componentName{ "Scriptable" };

	Halley::HashMap<Halley::String, std::shared_ptr<Halley::ScriptState>> activeStates{};
	Halley::Vector<Halley::String> tags{};
	Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>> scripts{};
	Halley::Vector<Halley::String> scriptsStarted{};
	Halley::ScriptVariables variables{};

	ScriptableComponent() {
	}

	ScriptableComponent(Halley::Vector<Halley::String> tags, Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>> scripts)
		: tags(std::move(tags))
		, scripts(std::move(scripts))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::serialize(activeStates, Halley::HashMap<Halley::String, std::shared_ptr<Halley::ScriptState>>{}, _context, _node, componentName, "activeStates", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::serialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scripts)>::serialize(scripts, Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>>{}, _context, _node, componentName, "scripts", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(variables)>::serialize(variables, Halley::ScriptVariables{}, _context, _node, componentName, "variables", makeMask(Type::SaveData, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::deserialize(activeStates, Halley::HashMap<Halley::String, std::shared_ptr<Halley::ScriptState>>{}, _context, _node, componentName, "activeStates", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::deserialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scripts)>::deserialize(scripts, Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>>{}, _context, _node, componentName, "scripts", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(variables)>::deserialize(variables, Halley::ScriptVariables{}, _context, _node, componentName, "variables", makeMask(Type::SaveData, Type::Network));
	}

};
