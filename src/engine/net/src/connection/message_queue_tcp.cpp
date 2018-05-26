#include "halley/net/connection/message_queue_tcp.h"
using namespace Halley;

MessageQueueTCP::MessageQueueTCP(std::shared_ptr<IConnection> connection)
	: connection(connection)
{
}

void MessageQueueTCP::enqueue(std::unique_ptr<NetworkMessage> msg, int channel)
{
}

void MessageQueueTCP::sendAll()
{
}

std::vector<std::unique_ptr<NetworkMessage>> MessageQueueTCP::receiveAll()
{
	return {};
}
