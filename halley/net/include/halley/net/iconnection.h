#pragma once

namespace Halley
{
	class InboundNetworkPacket;
	class OutboundNetworkPacket;

	enum class ConnectionStatus
	{
		UNDEFINED,
		OPEN,
		CLOSING,
		CLOSED
	};

	class IConnection
	{
	public:
		virtual ~IConnection() {}

		virtual void close() = 0;
		virtual ConnectionStatus getStatus() const = 0;
		virtual void send(OutboundNetworkPacket&& packet) = 0;
		virtual bool receive(InboundNetworkPacket& packet) = 0;
	};
}
