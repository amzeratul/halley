// Halley codegen version 124
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class PlayNetworkSoundSystemMessage final : public Halley::SystemMessage {
public:
	static constexpr int messageIndex{ 0 };
	static const constexpr char* messageName{ "PlayNetworkSound" };
	static constexpr Halley::SystemMessageDestination messageDestination{ Halley::SystemMessageDestination::AllClients };
	using ReturnType = void;

	Halley::EntityId emitter{};
	Halley::String event{};

	PlayNetworkSoundSystemMessage() {
	}

	PlayNetworkSoundSystemMessage(Halley::EntityId emitter, Halley::String event)
		: emitter(std::move(emitter))
		, event(std::move(event))
	{
	}

	size_t getSize() const override final {
		return sizeof(PlayNetworkSoundSystemMessage);
	}

	int getId() const override final {
		return messageIndex;
	}
	Halley::SystemMessageDestination getMessageDestination() const override final {
		return messageDestination;
	}


	void serialize(Halley::Serializer& s) const override final {
		s << emitter;
		s << event;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> emitter;
		s >> event;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(emitter)>::deserialize(emitter, Halley::EntityId{}, context, node, "", "emitter", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(event)>::deserialize(event, Halley::String{}, context, node, "", "event", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}
};
