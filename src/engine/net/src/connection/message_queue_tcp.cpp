#include <utility>

#include "halley/net/connection/message_queue_tcp.h"
#include "connection/network_packet.h"
using namespace Halley;

MessageQueueTCP::MessageQueueTCP(std::shared_ptr<IConnection> connection)
	: connection(std::move(connection))
{
}

MessageQueueTCP::~MessageQueueTCP()
{
	connection->close();
	connection.reset();
}

bool MessageQueueTCP::isConnected() const
{
	return connection->getStatus() == ConnectionStatus::Connected || connection->getStatus() == ConnectionStatus::Connecting;
}

void MessageQueueTCP::enqueue(OutboundNetworkPacket packet, uint8_t channel)
{
	if (isConnected()) {
		connection->send(IConnection::TransmissionType::Reliable, std::move(packet));
	}
}

void MessageQueueTCP::sendAll()
{
}

Vector<InboundNetworkPacket> MessageQueueTCP::receivePackets()
{
	Vector<InboundNetworkPacket> result;

	if (isConnected()) {
		InboundNetworkPacket packet;
		while (connection->receive(packet)) {
			result.push_back(std::move(packet));
		}
	}

	return result;
}
