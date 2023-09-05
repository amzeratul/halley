// Halley codegen version 124
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class NetworkEntityLockSystemMessage final : public Halley::SystemMessage {
public:
	static constexpr int messageIndex{ 4 };
	static const constexpr char* messageName{ "NetworkEntityLock" };
	static constexpr Halley::SystemMessageDestination messageDestination{ Halley::SystemMessageDestination::Host };
	using ReturnType = bool;

	Halley::EntityId target{};
	bool lock{};
	uint8_t peerId{};

	NetworkEntityLockSystemMessage() {
	}

	NetworkEntityLockSystemMessage(Halley::EntityId target, bool lock, uint8_t peerId)
		: target(std::move(target))
		, lock(std::move(lock))
		, peerId(std::move(peerId))
	{
	}

	size_t getSize() const override final {
		return sizeof(NetworkEntityLockSystemMessage);
	}

	int getId() const override final {
		return messageIndex;
	}
	Halley::SystemMessageDestination getMessageDestination() const override final {
		return messageDestination;
	}


	void serialize(Halley::Serializer& s) const override final {
		s << target;
		s << lock;
		s << peerId;
	}

	void deserialize(Halley::Deserializer& s) override final {
		s >> target;
		s >> lock;
		s >> peerId;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) override final {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(target)>::deserialize(target, Halley::EntityId{}, context, node, "", "target", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(lock)>::deserialize(lock, bool{}, context, node, "", "lock", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(peerId)>::deserialize(peerId, uint8_t{}, context, node, "", "peerId", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}
};
