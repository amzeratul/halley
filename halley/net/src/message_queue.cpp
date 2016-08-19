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
	std::vector<std::unique_ptr<IMessage>> result;

	InboundNetworkPacket packet;
	while (connection->receive(packet)) {
		// TODO: deserialize messages
	}

	return result;
}

void MessageQueue::enqueue(std::unique_ptr<IMessage> msg, int channel)
{
	auto i = channels.find(channel);
	if (i == channels.end()) {
		throw Exception("Channel " + String::integerToString(channel) + " has not been set up");
	}

	pendingMsgs.push_back(std::move(msg));
}

void MessageQueue::sendAll()
{
	while (!pendingMsgs.empty()) {
		std::vector<std::unique_ptr<IMessage>> sentMsgs;
		std::array<gsl::byte, 1500> buffer;
		gsl::span<gsl::byte> dst(buffer);
		size_t size = 0;

		// Figure out what messages are going in this packet
		auto next = pendingMsgs.begin();
		for (auto iter = pendingMsgs.begin(); iter != pendingMsgs.end(); iter = next) {
			++next;

			size_t msgSize = (*iter)->getSerializedSize(); // TODO: include header

			// Message fits, move to list
			if (size + msgSize <= buffer.size()) {
				(*iter)->serializeTo(dst.subspan(size, msgSize));
				size += msgSize;
				sentMsgs.push_back(std::move(*iter));
				pendingMsgs.erase(iter);
			}
		}

		if (sentMsgs.empty()) {
			throw Exception("Was not able to fit any messages into packet!");
		}

		// Track data in this packet
		int tag = nextPacketId++;
		auto& pendingData = pendingPackets[tag];
		pendingData.msgs = std::move(sentMsgs);
		pendingData.timeSent = std::chrono::steady_clock::now();

		// Send
		connection->sendTagged(OutboundNetworkPacket(dst.first(size)), tag);
	}
}

void MessageQueue::onPacketAcked(int tag)
{
	auto i = pendingPackets.find(tag);
	if (i != pendingPackets.end()) {
		auto& packet = i->second;

		for (auto& m : packet.msgs) {
			// TODO: ack messages
		}

		// Remove pending
		pendingPackets.erase(tag);
	}
}
