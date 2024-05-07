// Halley codegen version 128
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class EmbeddedScriptComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 12 };
	static const constexpr char* componentName{ "EmbeddedScript" };

	Halley::ScriptGraph script{};

	EmbeddedScriptComponent() {
	}

	EmbeddedScriptComponent(Halley::ScriptGraph script)
		: script(std::move(script))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(script)>::serialize(script, Halley::ScriptGraph{}, _context, _node, componentName, "script", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(script)>::deserialize(script, Halley::ScriptGraph{}, _context, _node, componentName, "script", makeMask(Type::Prefab));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<EmbeddedScriptComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<EmbeddedScriptComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<EmbeddedScriptComponent>(ptr);
	}

};
