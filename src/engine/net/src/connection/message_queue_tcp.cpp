#include "halley/net/connection/message_queue_tcp.h"
#include "connection/network_packet.h"
using namespace Halley;

MessageQueueTCP::MessageQueueTCP(std::shared_ptr<IConnection> connection)
	: connection(connection)
{
}

void MessageQueueTCP::enqueue(std::unique_ptr<NetworkMessage> msg, int channel)
{
	auto packet = OutboundNetworkPacket(Serializer::toBytes(*msg));
	packet.addHeader(getMessageType(*msg));
	connection->send(std::move(packet));
}

void MessageQueueTCP::sendAll()
{
}

std::vector<std::unique_ptr<NetworkMessage>> MessageQueueTCP::receiveAll()
{
	std::vector<std::unique_ptr<NetworkMessage>> result;

	InboundNetworkPacket packet;
	while (connection->receive(packet)) {
		int messageType = -1;
		packet.extractHeader(messageType);
		result.push_back(deserializeMessage(packet.getBytes(), messageType, 0));
	}

	return result;
}
