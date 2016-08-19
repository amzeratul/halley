#pragma once
#include <gsl/gsl>

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

	private:
		void setStream(gsl::not_null<IMessageStream*> stream);
		bool isReliable() const;
		void onAck();

		IMessageStream* stream;
	};
}
