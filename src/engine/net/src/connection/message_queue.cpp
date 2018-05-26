#include <halley/support/exception.h>
#include "connection/message_queue.h"
#include "connection/reliable_connection.h"
#include "halley/text/string_converter.h"

using namespace Halley;


MessageQueue::~MessageQueue()
{
}

void MessageQueue::setChannel(int channel, ChannelSettings settings)
{
}

void MessageQueue::addFactory(std::unique_ptr<NetworkMessageFactoryBase> factory)
{
	typeToMsgIndex[factory->getTypeIndex()] = int(factories.size());
	factories.emplace_back(std::move(factory));
}

int MessageQueue::getMessageType(NetworkMessage& msg) const
{
	auto idxIter = typeToMsgIndex.find(std::type_index(typeid(msg)));
	if (idxIter == typeToMsgIndex.end()) {
		throw Exception("No appropriate factory for this type of message: " + String(typeid(msg).name()));
	}
	return idxIter->second;
}

std::unique_ptr<NetworkMessage> MessageQueue::deserializeMessage(gsl::span<const gsl::byte> data, unsigned short msgType, unsigned short seq)
{
	auto msg = factories.at(msgType)->create(data);
	msg->seq = seq;
	return msg;
}

