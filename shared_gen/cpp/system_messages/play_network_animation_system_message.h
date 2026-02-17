// Halley codegen version 138
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class PlayNetworkAnimationSystemMessage final : public Halley::SystemMessage {
public:
	static constexpr int messageIndex{ 0 };
	static const constexpr char* messageName{ "PlayNetworkAnimation" };
	static constexpr Halley::SystemMessageDestination messageDestination{ Halley::SystemMessageDestination::RemoteClients };
	using ReturnType = void;

	Halley::EntityId entity{};
	Halley::String sequence{};
	bool once{};

	PlayNetworkAnimationSystemMessage() {
	}

	PlayNetworkAnimationSystemMessage(Halley::EntityId entity, Halley::String sequence, bool once)
		: entity(std::move(entity))
		, sequence(std::move(sequence))
		, once(std::move(once))
	{
	}

	size_t getSize() const override final {
		return sizeof(PlayNetworkAnimationSystemMessage);
	}

	int getId() const override final {
		return messageIndex;
	}
	Halley::SystemMessageDestination getMessageDestination() const override final {
		return messageDestination;
	}


	void serialize(Halley::Serializer& s) const override final {
		s << entity;
		s << sequence;
		s << once;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> entity;
		s >> sequence;
		s >> once;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(entity)>::deserialize(entity, Halley::EntityId{}, context, node, "", "entity", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(sequence)>::deserialize(sequence, Halley::String{}, context, node, "", "sequence", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(once)>::deserialize(once, bool{}, context, node, "", "once", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
	}
};
