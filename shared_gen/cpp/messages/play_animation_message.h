// Halley codegen version 123
#pragma once

#include <halley.hpp>


class PlayAnimationMessage final : public Halley::Message {
public:
	static constexpr int messageIndex{ 0 };
	static const constexpr char* messageName{ "PlayAnimation" };

	Halley::String sequence{};

	PlayAnimationMessage() {
	}

	PlayAnimationMessage(Halley::String sequence)
		: sequence(std::move(sequence))
	{
	}

	size_t getSize() const override final {
		return sizeof(PlayAnimationMessage);
	}

	int getId() const override final {
		return messageIndex;
	}
};
