#pragma once
#include <gsl/gsl>
#include <typeindex>

namespace Halley
{
	class IMessageStream;
	class MessageQueue;

	class NetworkMessage
	{
		friend class MessageQueue;

	public:
		virtual ~NetworkMessage() {}

		virtual size_t getSerializedSize() const = 0;
		virtual void serializeTo(gsl::span<gsl::byte> dst) = 0;
		virtual void deserializeFrom(gsl::span<const gsl::byte> src) = 0;

	private:
		int channel = -1;
		unsigned int seq = 0;
	};

	class NetworkMessageFactoryBase
	{
	public:
		virtual ~NetworkMessageFactoryBase() {}

		virtual std::unique_ptr<NetworkMessage> create() const = 0;
		virtual std::type_index getTypeIndex() const = 0;
	};

	template <typename T>
	class NetworkMessageFactory : public NetworkMessageFactoryBase
	{
	public:
		std::unique_ptr<NetworkMessage> create() const override
		{
			return std::make_unique<T>();
		}

		std::type_index getTypeIndex() const override
		{
			return std::type_index(typeid(T));
		}
	};
}
