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
		int channel = -1;
		unsigned int seq = 0;
	};
}
