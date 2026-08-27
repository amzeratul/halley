// Halley codegen version 147
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/bytes/byte_serializer.h"
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class SpriteAnimationEventsComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 9 };
	static constexpr uint32_t quickIndex{ 0 };
	static const constexpr char* componentName{ "SpriteAnimationEvents" };
	static constexpr bool alwaysEnabled{ false };

	Halley::Vector<Halley::String> tags{};
	uint32_t prevAnimIdx{ 0 };
	int prevSeqId{ -1 };
	int prevDir{ -1 };
	int prevFrame{ -1 };

	SpriteAnimationEventsComponent() {
	}

	SpriteAnimationEventsComponent(Halley::Vector<Halley::String> tags)
		: tags(std::move(tags))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(tags)>::serialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab));
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(tags)>::deserialize(tags, Halley::Vector<Halley::String>{}, _context, _node, componentName, "tags", makeMask(Type::Prefab));
	}

	void hash(const Halley::EntitySerializationContext& _context, Halley::Hash::Hasher& _hasher) const {
		using namespace Halley::EntitySerialization;
		
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		if ((_mask & makeMask(Type::Prefab)) == 0) _node.removeKey("tags");
	}

	Halley::ConfigNode serializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName) const {
		
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void deserializeField(const Halley::EntitySerializationContext& _context, std::string_view _fieldName, const Halley::ConfigNode& _node) {
		
		throw Halley::Exception("Unknown or non-serializable field \"" + Halley::String(_fieldName) + "\"", Halley::HalleyExceptions::Entity);
	}

	void serializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Serializer& _serializer) const {
		
	}

	void deserializeNetwork(const Halley::ByteSerializationContext& _context, Halley::Deserializer& _deserializer) {
		
	}


	void* operator new(std::size_t size, std::align_val_t align) {
		return doNew<SpriteAnimationEventsComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<SpriteAnimationEventsComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<SpriteAnimationEventsComponent>(ptr);
	}

};
