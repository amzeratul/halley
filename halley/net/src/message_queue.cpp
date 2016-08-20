#include "message_queue.h"
#include "reliable_connection.h"
#include <halley/support/exception.h>
#include <network_packet.h>

using namespace Halley;

ChannelSettings::ChannelSettings(bool reliable, bool ordered, bool keepLastSent)
	: reliable(reliable)
	, ordered(ordered)
	, keepLastSent(keepLastSent)
{}

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

void MessageQueue::setChannel(int channel, ChannelSettings settings)
{
	Expects(channel >= 0 && channel <= 255);

	if (channels.find(channel) != channels.end()) {
		throw Exception("Channel " + String::integerToString(channel) + " already set");
	}

	auto& c = channels[channel];
	c.settings = settings;
}

std::vector<std::unique_ptr<NetworkMessage>> MessageQueue::receiveAll()
{
	std::vector<std::unique_ptr<NetworkMessage>> result;

	InboundNetworkPacket packet;
	while (connection->receive(packet)) {
		// TODO: deserialize messages
	}

	return result;
}

void MessageQueue::enqueue(std::unique_ptr<NetworkMessage> msg, int channel)
{
	auto i = channels.find(channel);
	if (i == channels.end()) {
		throw Exception("Channel " + String::integerToString(channel) + " has not been set up");
	}

	msg->channel = channel;

	pendingMsgs.push_back(std::move(msg));
}

void MessageQueue::sendAll()
{
	checkReSend();

	while (!pendingMsgs.empty()) {
		std::vector<std::unique_ptr<NetworkMessage>> sentMsgs;
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
			// TODO
		}

		// Remove pending
		pendingPackets.erase(tag);
	}
}

void MessageQueue::checkReSend()
{
	auto next = pendingPackets.begin();
	for (auto iter = pendingPackets.begin(); iter != pendingPackets.end(); iter = next) {
		++next;
		auto& pending = iter->second;

		// Check how long it's been waiting
		float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - pending.timeSent).count();
		if (elapsed > 0.1f && elapsed > connection->getLatency() * 2.0f) {
			// Re-send any reliable messages
			for (auto& m : pending.msgs) {
				if (channels[m->channel].settings.reliable) {
					pendingMsgs.push_back(std::move(m));
				}
			}
			pendingPackets.erase(iter);
		}
	}
}
