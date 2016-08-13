#pragma once

namespace Halley
{
	class NetworkPacket;

	class IConnection
	{
	public:
		virtual ~IConnection() {}
		virtual void close() = 0;
		virtual void send(NetworkPacket&& packet) = 0;
		virtual bool receive(NetworkPacket& packet) = 0;
	};
}