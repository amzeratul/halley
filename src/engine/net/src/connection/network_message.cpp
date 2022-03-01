#include "connection/network_message.h"

using namespace Halley;


void NetworkMessageFactories::addFactory(std::unique_ptr<NetworkMessageFactoryBase> factory)
{
	typeToMsgIndex[factory->getTypeIndex()] = static_cast<uint16_t>(factories.size());
	factories.emplace_back(std::move(factory));
}

uint16_t NetworkMessageFactories::getMessageType(NetworkMessage& msg) const
{
	auto idxIter = typeToMsgIndex.find(std::type_index(typeid(msg)));
	if (idxIter == typeToMsgIndex.end()) {
		throw Exception("No appropriate factory for this type of message: " + String(typeid(msg).name()), HalleyExceptions::Network);
	}
	return idxIter->second;
}

std::unique_ptr<NetworkMessage> NetworkMessageFactories::deserializeMessage(gsl::span<const gsl::byte> data, uint16_t msgType, uint16_t seq)
{
	auto msg = factories.at(msgType)->create(data);
	msg->setSeq(seq);
	return msg;
}
