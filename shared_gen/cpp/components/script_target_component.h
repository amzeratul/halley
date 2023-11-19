// Halley codegen version 127
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif
#include "halley/support/exception.h"


class ScriptTargetComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 13 };
	static const constexpr char* componentName{ "ScriptTarget" };

	Halley::String id{};

	ScriptTargetComponent() {
	}

	ScriptTargetComponent(Halley::String id)
		: id(std::move(id))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(id)>::serialize(id, Halley::String{}, _context, _node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(id)>::deserialize(id, Halley::String{}, _context, _node, componentName, "id", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "id") {
			return Halley::ConfigNodeHelper<decltype(id)>::serialize(id, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "id") {
			Halley::ConfigNodeHelper<decltype(id)>::deserialize(id, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<ScriptTargetComponent>(size, align);
	}

	void operator delete(void* ptr) {
		return doDelete<ScriptTargetComponent>(ptr);
	}

};
