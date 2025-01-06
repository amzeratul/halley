#pragma once

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

		virtual String getRemoteAddress() const { return String("0.0.0.0:0"); }

        /* 2nd, very minimal interface to send/receive unreliable packets. */

        class IPacketListener
        {
        public:
            virtual void onSend(gsl::span<const gsl::byte> packet) = 0;
            virtual void onReceive(gsl::span<const gsl::byte> packet) = 0;
        };

        [[nodiscard]] virtual size_t getMaxUnreliablePacketSize() const { return 0; }

        virtual void onConnect(short connId) {}

        virtual void sendUnreliablePacket(gsl::span<const gsl::byte> packet) {}

        virtual void setUnreliablePacketListener(IPacketListener* listener) {}
	};
}
