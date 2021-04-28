#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class ScriptTargetComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 9 };
	static const constexpr char* componentName{ "ScriptTarget" };


	ScriptTargetComponent() {
	}

	Halley::ConfigNode serialize(const Halley::ConfigNodeSerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		
		return node;
	}

	void deserialize(const Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		
	}

};
