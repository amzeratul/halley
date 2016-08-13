#pragma once

namespace Halley
{
	class NetworkPacket;

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
		virtual void send(NetworkPacket&& packet) = 0;
		virtual bool receive(NetworkPacket& packet) = 0;
	};
}
