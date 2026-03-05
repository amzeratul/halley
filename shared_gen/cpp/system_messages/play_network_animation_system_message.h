// Halley codegen version 139
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
	bool reverse{};
	bool once{};
	Halley::String direction{};

	PlayNetworkAnimationSystemMessage() {
	}

	PlayNetworkAnimationSystemMessage(Halley::EntityId entity, Halley::String sequence, bool reverse, bool once, Halley::String direction)
		: entity(std::move(entity))
		, sequence(std::move(sequence))
		, reverse(std::move(reverse))
		, once(std::move(once))
		, direction(std::move(direction))
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
		s << reverse;
		s << once;
		s << direction;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> entity;
		s >> sequence;
		s >> reverse;
		s >> once;
		s >> direction;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(entity)>::deserialize(entity, Halley::EntityId{}, context, node, "", "entity", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(sequence)>::deserialize(sequence, Halley::String{}, context, node, "", "sequence", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(reverse)>::deserialize(reverse, bool{}, context, node, "", "reverse", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(once)>::deserialize(once, bool{}, context, node, "", "once", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
		Halley::EntityConfigNodeSerializer<decltype(direction)>::deserialize(direction, Halley::String{}, context, node, "", "direction", makeMask(Type::Prefab, Type::SaveData, Type::Network, Type::Dynamic));
	}
};
