#pragma once
#include <gsl/gsl>
#include <typeindex>
#include "halley/utils/utils.h"
#include "halley/data_structures/maybe.h"
#include "halley/bytes/byte_serializer.h"
#include <cstdint>

namespace Halley
{
	class IMessageStream;
	class MessageQueue;
	class MessageQueueUDP;
	class MessageQueueTCP;
	
	class NetworkMessage
	{
	public:
		virtual ~NetworkMessage() = default;

		size_t getSerializedSize() const
		{
			if (!serialized) {
				serialized = Serializer::toBytes(*this, getSerializerOptions());
			}
			return serialized->size();
		}

		void serializeTo(gsl::span<gsl::byte> dst) const
		{
			if (!serialized) {
				serialized = Serializer::toBytes(*this, getSerializerOptions());
			}
			memcpy(dst.data(), serialized->data(), serialized->size());
		}

		virtual void serialize(Serializer& s) const = 0;
		virtual void deserialize(Deserializer& s) = 0;

		void setSeq(uint16_t seq) { this->seq = seq; }
		uint16_t getSeq() const { return seq; }
		void setChannel(uint8_t channel) { this->channel = channel; }
		uint8_t getChannel() const { return channel; }

		static SerializerOptions getSerializerOptions()
		{
			return SerializerOptions(SerializerOptions::maxVersion);
		}

	private:
		uint16_t seq = 0;
		uint8_t channel = -1;

		mutable std::optional<Bytes> serialized;
	};

	class NetworkMessageFactoryBase
	{
	public:
		virtual ~NetworkMessageFactoryBase() {}

		virtual std::unique_ptr<NetworkMessage> create(gsl::span<const gsl::byte> src) const = 0;
		virtual std::type_index getTypeIndex() const = 0;
	};

	template <typename T>
	class NetworkMessageFactory : public NetworkMessageFactoryBase
	{
	public:
		std::unique_ptr<NetworkMessage> create(gsl::span<const gsl::byte> src) const override
		{
			auto result = std::make_unique<T>();
			auto s = Deserializer(src, T::getSerializerOptions());
			result->deserialize(s);
			return result;
		}

		std::type_index getTypeIndex() const override
		{
			return std::type_index(typeid(T));
		}
	};

	class NetworkMessageFactories
	{
	public:
		template <typename T>
		void addFactory()
		{
			addFactory(std::make_unique<NetworkMessageFactory<T>>());
		}

		uint16_t getMessageType(NetworkMessage& msg) const;
		std::unique_ptr<NetworkMessage> deserializeMessage(gsl::span<const gsl::byte> data, uint16_t msgType, uint16_t seq);

	private:
		std::map<std::type_index, uint16_t> typeToMsgIndex;
		Vector<std::unique_ptr<NetworkMessageFactoryBase>> factories;

		void addFactory(std::unique_ptr<NetworkMessageFactoryBase> factory);
	};
}
