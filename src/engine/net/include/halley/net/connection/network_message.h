#pragma once
#include <gsl/gsl>
#include <typeindex>
#include "halley/utils/utils.h"
#include "halley/data_structures/maybe.h"
#include "halley/bytes/byte_serializer.h"

namespace Halley
{
	class IMessageStream;
	class MessageQueue;
	class MessageQueueUDP;
	class MessageQueueTCP;

	class NetworkMessage
	{
		friend class MessageQueue;
		friend class MessageQueueUDP;
		friend class MessageQueueTCP;

	public:
		virtual ~NetworkMessage() = default;

		size_t getSerializedSize() const
		{
			if (!serialized) {
				serialized = Serializer::toBytes(*this);
			}
			return serialized.get().size();
		}

		void serializeTo(gsl::span<gsl::byte> dst) const
		{
			if (!serialized) {
				serialized = Serializer::toBytes(*this);
			}
			memcpy(dst.data(), serialized.get().data(), serialized.get().size());
		}

		virtual void serialize(Serializer& s) const = 0;

	private:
		unsigned short seq = 0;
		char channel = -1;

		mutable Maybe<Bytes> serialized;
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
			return std::make_unique<T>(src);
		}

		std::type_index getTypeIndex() const override
		{
			return std::type_index(typeid(T));
		}
	};
}
