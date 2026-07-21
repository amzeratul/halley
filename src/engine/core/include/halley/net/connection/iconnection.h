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

		virtual UniqueLock<Mutex> lockSend() { return UniqueLock<Mutex>(); }
		virtual void send(TransmissionType type, OutboundNetworkPacket packet) = 0;

		virtual UniqueLock<Mutex> lockReceive() { return UniqueLock<Mutex>(); }
		virtual bool receive(InboundNetworkPacket& packet) = 0;

		[[nodiscard]] virtual String getRemoteAddress() const { return {"0.0.0.0:0"}; }

        /* 2nd, very minimal interface to send/receive unreliable packets. */

        class IPacketListener
        {
        public:
			virtual ~IPacketListener() = default;
			// NB: This may be called from a different thread on some platforms.
            virtual void onReceive(gsl::span<const std::byte> packet) = 0;
        };

        [[nodiscard]] virtual size_t getMaxUnreliablePacketSize() const { return 0; }

		// By default, resend un-ACK'd packets after <latency * 1.5> seconds.
		[[nodiscard]] virtual float getUnreliablePacketResendTime(float averageAckTime) { return averageAckTime * 1.5f; }

        virtual void onConnect(short connId) {}

        virtual void sendUnreliablePacket(gsl::span<const std::byte> packet) {}
		virtual void flushSendUnreliablePackets() {}

        virtual void setUnreliablePacketListener(IPacketListener* listener) {}
	};
}
