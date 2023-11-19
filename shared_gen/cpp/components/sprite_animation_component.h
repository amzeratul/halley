// Halley codegen version 127
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif
#include "halley/support/exception.h"


class SpriteAnimationComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 5 };
	static const constexpr char* componentName{ "SpriteAnimation" };

	Halley::AnimationPlayer player{};

	SpriteAnimationComponent() {
	}

	SpriteAnimationComponent(Halley::AnimationPlayer player)
		: player(std::move(player))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(player)>::serialize(player, Halley::AnimationPlayer{}, _context, _node, componentName, "player", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(player)>::deserialize(player, Halley::AnimationPlayer{}, _context, _node, componentName, "player", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic, Type::Network));
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "player") {
			return Halley::ConfigNodeHelper<decltype(player)>::serialize(player, _context);
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		if (_fieldName == "player") {
			Halley::ConfigNodeHelper<decltype(player)>::deserialize(player, _context, _node);
			return;
		}
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<SpriteAnimationComponent>(size, align);
	}

	void operator delete(void* ptr) {
		return doDelete<SpriteAnimationComponent>(ptr);
	}

};
