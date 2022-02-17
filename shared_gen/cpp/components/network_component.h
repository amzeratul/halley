#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class NetworkComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 10 };
	static const constexpr char* componentName{ "Network" };

	std::optional<uint8_t> ownerId{};
	Halley::DataInterpolatorSet dataInterpolatorSet{};

	NetworkComponent() {
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		
	}

};
