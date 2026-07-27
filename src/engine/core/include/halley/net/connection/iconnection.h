#pragma once

#include "halley/concurrency/mutex.h"
#include "halley/text/halleystring.h"

namespace Halley
{
	class InboundNetworkPacket;
	class OutboundNetworkPacket;

	enum class ConnectionStatus
	{
		Undefined,
		Connecting,
		Connected,
		Closing,
		Closed
	};

	class IConnection
	{
	public:
		enum class TransmissionType {
			Unreliable,
			Reliable
		};
		
		virtual ~IConnection() = default;

		virtual void close() = 0;
		[[nodiscard]] virtual ConnectionStatus getStatus() const = 0;

		[[nodiscard]] virtual bool isSupported(TransmissionType type) const = 0;

		virtual void send(TransmissionType type, OutboundNetworkPacket packet) = 0;
		virtual bool receive(InboundNetworkPacket& packet) = 0;

		[[nodiscard]] virtual String getRemoteAddress() const { return {"0.0.0.0:0"}; }

        /* 2nd, very minimal interface to send/receive unreliable packets. */

        class IPacketListener
        {
        public:
			virtual ~IPacketListener() = default;
            virtual void onUnreliablePacketReceived(gsl::span<const std::byte> packet) = 0;
			virtual void onUnreliablePacketAck(uint64_t id) = 0;
			virtual void onUnreliablePacketLost(uint64_t id) = 0;
        };

		// Returns a platform-specific, immutable value. This used to size various memory buffers.
        [[nodiscard]] virtual size_t getMaxUnreliablePacketSize() const { return 0; }

		// Returns the maximum packet size that is *actually* usable by an active connection.
		// Must be less or equal getMaxUnreliablePacketSize().
		[[nodiscard]] virtual size_t getRealMaxUnreliablePacketSize() const { return getMaxUnreliablePacketSize(); }

		// By default, resend un-ACK'd packets after <latency * 1.5> seconds.
		[[nodiscard]] virtual float getUnreliablePacketResendTime(float averageAckTime) { return averageAckTime * 1.5f; }
		[[nodiscard]] virtual bool doesInternalPacketAck() const { return false; }

        virtual void onConnect(short connId) {}

		virtual void beginSendUnreliablePackets() {}
        virtual void sendUnreliablePacket(gsl::span<const std::byte> packet, uint64_t id) {}
		virtual void flushSendUnreliablePackets() {}

        virtual void setUnreliablePacketListener(IPacketListener* listener) {}
	};
}
