#include "reliable_connection.h"

using namespace Halley;

ReliableConnection::ReliableConnection(std::shared_ptr<IConnection> parent)
	: parent(parent)
{
}

void ReliableConnection::close()
{
	parent->close();
}

ConnectionStatus ReliableConnection::getStatus() const
{
	return parent->getStatus();
}

void ReliableConnection::send(NetworkPacket&& packet)
{
	// TODO
	parent->send(std::move(packet));
}

bool ReliableConnection::receive(NetworkPacket& packet)
{
	return parent->receive(packet);
}
