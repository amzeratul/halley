// Halley codegen version 127
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif
#include "halley/support/exception.h"


class ScriptableComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 11 };
	static const constexpr char* componentName{ "Scriptable" };

	Halley::ScriptStateSet activeStates{};
	Halley::Vector<Halley::String> tags{};
	Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>> scripts{};
	Halley::Vector<Halley::String> scriptsStarted{};
	Halley::ScriptVariables variables{};
	Halley::HashMap<Halley::String, Halley::EntityId> entityReferences{};

	ScriptableComponent() {
	}

	ScriptableComponent(Halley::Vector<Halley::String> tags, Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>> scripts, Halley::HashMap<Halley::String, Halley::EntityId> entityReferences)
		: tags(std::move(tags))
		, scripts(std::move(scripts))
		, entityReferences(std::move(entityReferences))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::serialize(activeStates, Halley::ScriptStateSet{}, _context, _node, componentName, "activeStates", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::serialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scripts)>::serialize(scripts, Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>>{}, _context, _node, componentName, "scripts", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(variables)>::serialize(variables, Halley::ScriptVariables{}, _context, _node, componentName, "variables", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(entityReferences)>::serialize(entityReferences, Halley::HashMap<Halley::String, Halley::EntityId>{}, _context, _node, componentName, "entityReferences", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(activeStates)>::deserialize(activeStates, Halley::ScriptStateSet{}, _context, _node, componentName, "activeStates", makeMask(Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(tags)>::deserialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(scripts)>::deserialize(scripts, Halley::Vector<Halley::ResourceReference<Halley::ScriptGraph>>{}, _context, _node, componentName, "scripts", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(variables)>::deserialize(variables, Halley::ScriptVariables{}, _context, _node, componentName, "variables", makeMask(Type::SaveData, Type::Dynamic, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(entityReferences)>::deserialize(entityReferences, Halley::HashMap<Halley::String, Halley::EntityId>{}, _context, _node, componentName, "entityReferences", makeMask(Type::Prefab));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
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


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<ScriptableComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<ScriptableComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<ScriptableComponent>(ptr);
	}

};
