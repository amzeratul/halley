#include "reliable_connection.h"
#include <network_packet.h>

using namespace Halley;

struct ReliableHeader
{
	unsigned short sequence;
	unsigned short ack;
	unsigned int ackBits;
};

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
	ReliableHeader header;
	// TODO: fill header

	packet.addHeader(gsl::span<ReliableHeader>(&header, 1));
	parent->send(std::move(packet));
}

bool ReliableConnection::receive(NetworkPacket& packet)
{
	bool result = parent->receive(packet);
	if (result) {
		ReliableHeader header;
		packet.extractHeader(gsl::span<ReliableHeader>(&header, 1));

		// TODO: read header
	}

	return result;
}
