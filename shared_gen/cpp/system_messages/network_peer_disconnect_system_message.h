// Halley codegen version 138
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class NetworkPeerDisconnectSystemMessage final : public Halley::SystemMessage {
public:
	static constexpr int messageIndex{ 5 };
	static const constexpr char* messageName{ "NetworkPeerDisconnect" };
	static constexpr Halley::SystemMessageDestination messageDestination{ Halley::SystemMessageDestination::Local };
	using ReturnType = void;

	uint8_t peerId{};

	NetworkPeerDisconnectSystemMessage() {
	}

	NetworkPeerDisconnectSystemMessage(uint8_t peerId)
		: peerId(std::move(peerId))
	{
	}

	size_t getSize() const override final {
		return sizeof(NetworkPeerDisconnectSystemMessage);
	}

	int getId() const override final {
		return messageIndex;
	}
	Halley::SystemMessageDestination getMessageDestination() const override final {
		return messageDestination;
	}

};
