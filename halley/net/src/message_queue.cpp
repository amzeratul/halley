#include "message_queue.h"
#include "reliable_connection.h"
#include <halley/support/exception.h>
#include <network_packet.h>

using namespace Halley;

MessageQueue::MessageQueue(std::shared_ptr<ReliableConnection> connection)
	: connection(connection)
{
	Expects(connection);
	connection->addAckListener(*this);
}

MessageQueue::~MessageQueue()
{
	connection->removeAckListener(*this);
}

void MessageQueue::addStream(std::unique_ptr<IMessageStream> stream, int channel)
{
	Expects(channel >= 0 && channel <= 255);

	if (channels.find(channel) != channels.end()) {
		throw Exception("Channel " + String::integerToString(channel) + " already set");
	}

	channels[channel] = std::move(stream);
}

std::vector<std::unique_ptr<IMessage>> MessageQueue::receiveAll()
{
	// TODO
	return std::vector<std::unique_ptr<IMessage>>();
}

void MessageQueue::enqueue(IMessage&& msg, int channel)
{
	// TODO
	auto i = channels.find(channel);
	if (i == channels.end()) {
		throw Exception("Channel " + String::integerToString(channel) + " has not been set up");
	}

	size_t size = msg.getSerializedSize();
}

void MessageQueue::sendAll()
{
	// TODO
	std::array<gsl::byte, 1500> buffer;
	size_t size = 0;

	int id = nextPacketId++;
	connection->sendTagged(OutboundNetworkPacket(gsl::span<gsl::byte>(buffer).first(size)), id);
}

void MessageQueue::onPacketAcked(int tag)
{
	auto i = pendingPackets.find(tag);
	if (i != pendingPackets.end()) {
		auto& packet = i->second;

		// TODO
	}
}
