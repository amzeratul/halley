#pragma once

#include <halley/text/halleystring.h>
#include <halley/utils/utils.h>

namespace Halley
{
	enum class ConnectionStatus
	{
		DISCONNECTED,
		CONNECTING,
		PENDING_ACCEPT,
		OPEN
	};

	enum class NetworkMessageType
	{
		CONNECT,
		ACCEPT_CONNECTION,
		DISCONNECT,
		ACK,
		DATA
	};

	using NetworkMessage = Bytes;

	class UDPConnection
	{
	public:
		UDPConnection();
		UDPConnection(String address, int port);

		void accept();
		void close();

		void update();
		ConnectionStatus getStatus() const { return status; }

		bool sendData(int channel, NetworkMessage&& msg);
		
	private:
		ConnectionStatus status = ConnectionStatus::DISCONNECTED;

		bool send(int channel, NetworkMessageType type, NetworkMessage&& msg);
	};
}
