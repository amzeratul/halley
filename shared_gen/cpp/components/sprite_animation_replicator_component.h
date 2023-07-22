// Halley codegen version 123
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class SpriteAnimationReplicatorComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 7 };
	static const constexpr char* componentName{ "SpriteAnimationReplicator" };


	SpriteAnimationReplicatorComponent() {
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& _context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode _node = Halley::ConfigNode::MapType();
		
		return _node;
	}

	void deserialize(const Halley::EntitySerializationContext& _context, const Halley::ConfigNode& _node) {
		using namespace Halley::EntitySerialization;
		
	}

};
