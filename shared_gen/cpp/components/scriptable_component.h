// Halley codegen version 125
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif
#include "halley/support/exception.h"


class ScriptableComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 11 };
	static const constexpr char* componentName{ "Scriptable" };

	int64_t curScriptId{ 0 };
	Halley::HashMap<int64_t, std::shared_ptr<Halley::ScriptState>> activeStates{};
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
		Halley::EntityConfigNodeSerializer<decltype(curScriptId)>::serialize(curScriptId, int64_t{ 0 }, _context, _node, componentName, "curScriptId", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::serialize(activeStates, Halley::HashMap<int64_t, std::shared_ptr<Halley::ScriptState>>{}, _context, _node, componentName, "activeStates", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::serialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scripts)>::serialize(scripts, Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>>{}, _context, _node, componentName, "scripts", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(variables)>::serialize(variables, Halley::ScriptVariables{}, _context, _node, componentName, "variables", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(curScriptId)>::deserialize(curScriptId, int64_t{ 0 }, _context, _node, componentName, "curScriptId", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::deserialize(activeStates, Halley::HashMap<int64_t, std::shared_ptr<Halley::ScriptState>>{}, _context, _node, componentName, "activeStates", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::deserialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scripts)>::deserialize(scripts, Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>>{}, _context, _node, componentName, "scripts", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(variables)>::deserialize(variables, Halley::ScriptVariables{}, _context, _node, componentName, "variables", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "curScriptId") {
			return Halley::ConfigNodeHelper<decltype(curScriptId)>::serialize(curScriptId, _context);
		}
		if (_fieldName == "tags") {
			return Halley::ConfigNodeHelper<decltype(tags)>::serialize(tags, _context);
		}
		if (_fieldName == "variables") {
			return Halley::ConfigNodeHelper<decltype(variables)>::serialize(variables, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "curScriptId") {
			Halley::ConfigNodeHelper<decltype(curScriptId)>::deserialize(curScriptId, _context, _node);
			return;
		}
		if (_fieldName == "tags") {
			Halley::ConfigNodeHelper<decltype(tags)>::deserialize(tags, _context, _node);
			return;
		}
		if (_fieldName == "variables") {
			Halley::ConfigNodeHelper<decltype(variables)>::deserialize(variables, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

};
