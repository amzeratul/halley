#include <halley/support/exception.h>
#include "connection/message_queue.h"
#include "connection/ack_unreliable_connection.h"
#include "halley/text/string_converter.h"

using namespace Halley;


MessageQueue::~MessageQueue()
{
}

void MessageQueue::enqueue(std::unique_ptr<NetworkMessage> msg, uint16_t channel)
{
	auto packet = OutboundNetworkPacket(msg->getBytes());
	packet.addHeader(getMessageType(*msg));
	enqueue(std::move(packet), channel);
}

Vector<std::unique_ptr<NetworkMessage>> MessageQueue::receiveMessages()
{
	Vector<std::unique_ptr<NetworkMessage>> result;
	auto msgs = receiveRaw();
	result.reserve(msgs.size());

	for (auto& msg: msgs) {
		uint16_t messageType = 0;
		msg.extractHeader(messageType);
		result.push_back(deserializeMessage(msg.getBytes(), messageType, 0));
	}

	return result;
}

void MessageQueue::setChannel(int channel, ChannelSettings settings)
{
}

