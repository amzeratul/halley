#include "udp_connection.h"
#include <halley/support/exception.h>

using namespace Halley;

UDPConnection::UDPConnection()
{
}

UDPConnection::UDPConnection(String address, int port)
	: status(ConnectionStatus::CONNECTING)
{
	send(0, NetworkMessageType::CONNECT, NetworkMessage());
}

void UDPConnection::accept()
{
	send(0, NetworkMessageType::ACCEPT_CONNECTION, NetworkMessage());
	status = ConnectionStatus::OPEN;
}

void UDPConnection::close()
{
	send(0, NetworkMessageType::DISCONNECT, NetworkMessage());
	status = ConnectionStatus::DISCONNECTED;
}

void UDPConnection::update()
{
	// TODO
}

bool UDPConnection::sendData(int channel, NetworkMessage&& msg)
{
	if (status != ConnectionStatus::OPEN) {
		return false;
	}
	if (channel == 0) {
		return false;
	}

	return send(channel, NetworkMessageType::DATA, std::move(msg));
}

bool UDPConnection::send(int channel, NetworkMessageType type, NetworkMessage&& msg)
{
	// TODO
	return false;
}
