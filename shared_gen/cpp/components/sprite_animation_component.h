// Halley codegen version 146
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/bytes/byte_serializer.h"
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class SpriteAnimationComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 5 };
	static constexpr uint32_t quickIndex{ 0 };
	static const constexpr char* componentName{ "SpriteAnimation" };
	static constexpr bool alwaysEnabled{ false };

	Halley::AnimationPlayer player{};
	bool updateSprite{ true };
	std::optional<Halley::Rect4f> cullBounds{};

	SpriteAnimationComponent() {
	}

	SpriteAnimationComponent(Halley::AnimationPlayer player, bool updateSprite, std::optional<Halley::Rect4f> cullBounds)
		: player(std::move(player))
		, updateSprite(std::move(updateSprite))
		, cullBounds(std::move(cullBounds))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(player)>::serialize(player, Halley::AnimationPlayer{}, _context, _node, componentName, "player", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(updateSprite)>::serialize(updateSprite, bool{ true }, _context, _node, componentName, "updateSprite", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(cullBounds)>::serialize(cullBounds, std::optional<Halley::Rect4f>{}, _context, _node, componentName, "cullBounds", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(player)>::deserialize(player, Halley::AnimationPlayer{}, _context, _node, componentName, "player", makeMask(Type::Prefab, Type::SaveData, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(updateSprite)>::deserialize(updateSprite, bool{ true }, _context, _node, componentName, "updateSprite", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(cullBounds)>::deserialize(cullBounds, std::optional<Halley::Rect4f>{}, _context, _node, componentName, "cullBounds", makeMask(Type::Prefab));
	}

	void hash(const Halley::EntitySerializationContext& _context, Halley::Hash::Hasher& _hasher) const {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(player)>::hash<makeMask(Type::Prefab, Type::SaveData, Type::Dynamic)>(_hasher, player, _context, "player");
		Halley::EntityConfigNodeSerializer<decltype(updateSprite)>::hash<makeMask(Type::Prefab)>(_hasher, updateSprite, _context, "updateSprite");
		Halley::EntityConfigNodeSerializer<decltype(cullBounds)>::hash<makeMask(Type::Prefab)>(_hasher, cullBounds, _context, "cullBounds");
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Prefab, Type::SaveData, Type::Dynamic)) == 0) _node.removeKey("player");
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("updateSprite");
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("cullBounds");
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

	void serializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Serializer& _serializer) const {
		
	}

	void deserializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Deserializer& _deserializer) {
		
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<SpriteAnimationComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<SpriteAnimationComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<SpriteAnimationComponent>(ptr);
	}

};
