// Halley codegen version 123
#pragma once

#include <halley.hpp>


class PlayAnimationOnceMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 1 };
	static const constexpr char* messageName{ "PlayAnimationOnce" };

	Halley::String sequence{};

	PlayAnimationOnceMessage() {
	}

	PlayAnimationOnceMessage(Halley::String sequence)
		: sequence(std::move(sequence))
	{
	}

	size_t getSize() const override final {
		return sizeof(PlayAnimationOnceMessage);
	}

	int getId() const override final {
		return messageIndex;
	}
};
