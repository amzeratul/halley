// Halley codegen version 150
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#else
#include "halley/bytes/byte_serializer.h"
#include "halley/entity/component.h"
#endif
#include "halley/support/exception.h"


class InvisiblePartitionTagComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 18 };
	static constexpr uint32_t quickIndex{ 16 };
	static const constexpr char* componentName{ "InvisiblePartitionTag" };
	static constexpr bool alwaysEnabled{ false };


	InvisiblePartitionTagComponent() {
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		
	}

	void hash(const Halley::EntitySerializationContext& _context, Halley::Hash::Hasher& _hasher) const {
		using namespace Halley::EntitySerialization;
		
	}

	static void sanitize(Halley::ConfigNode& _node, int _mask) {
		using namespace Halley::EntitySerialization;
		
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
		return doNew<InvisiblePartitionTagComponent>(size, align);
	}

	void* operator new(std::size_t size) {
		return doNew<InvisiblePartitionTagComponent>(size);
	}

	void operator delete(void* ptr) {
		return doDelete<InvisiblePartitionTagComponent>(ptr);
	}

};
