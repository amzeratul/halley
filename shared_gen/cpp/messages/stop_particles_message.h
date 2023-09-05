// Halley codegen version 124
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class StopParticlesMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 2 };
	static const constexpr char* messageName{ "StopParticles" };


	StopParticlesMessage() {
	}

	size_t getSize() const override final {
		return sizeof(StopParticlesMessage);
	}

	int getId() const override final {
		return messageIndex;
	}

	void serialize(Halley::Serializer& s) const override final {
		
	}

	void deserialize(Halley::Deserializer& s) override final {
		
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
	}
};
