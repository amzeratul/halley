#pragma once

namespace Halley
{
	class NetworkPacket;

	class IConnection
	{
	public:
		virtual void send(const NetworkPacket& packet) = 0;
		virtual bool receive(NetworkPacket& packet) = 0;
	};
}