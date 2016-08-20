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

void MessageQueue::enqueue(std::unique_ptr<NetworkMessage> msg, int channelNumber)
{
	auto i = channels.find(channelNumber);
	if (i == channels.end()) {
		throw Exception("Channel " + String::integerToString(channelNumber) + " has not been set up");
	}
	auto& channel = i->second;

	msg->channel = channelNumber;
	msg->seq = ++channel.lastSeq;

	pendingMsgs.push_back(std::move(msg));
}

void MessageQueue::sendAll()
{
	int firstTag = nextPacketId;
	std::vector<ReliableSubPacket> toSend;

	// Add packets which need to be re-sent
	checkReSend(toSend);

	// Create packets of pending messages
	while (!pendingMsgs.empty()) {
		toSend.emplace_back(createPacket());
	}

	// Send and update sequences
	connection->sendTagged(toSend);
	for (auto& pending: toSend) {
		pendingPackets[pending.tag].seq = pending.seq;
	}
}

void MessageQueue::onPacketAcked(int tag)
{
	auto i = pendingPackets.find(tag);
	if (i != pendingPackets.end()) {
		auto& packet = i->second;

		for (auto& m : packet.msgs) {
			auto& channel = channels[m->channel];
			if (m->seq - channel.lastAckSeq < 0x7FFFFFFF) {
				channel.lastAckSeq = m->seq;
				if (channel.settings.keepLastSent) {
					channel.lastAck = std::move(m);
				}
			}
		}

		// Remove pending
		pendingPackets.erase(tag);
	}
}

void MessageQueue::checkReSend(std::vector<ReliableSubPacket>& collect)
{
	auto next = pendingPackets.begin();
	for (auto iter = pendingPackets.begin(); iter != pendingPackets.end(); iter = next) {
		++next;
		auto& pending = iter->second;

		// Check how long it's been waiting
		float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - pending.timeSent).count();
		if (elapsed > 0.1f && elapsed > connection->getLatency() * 2.0f) {
			// Re-send if it's reliable
			if (pending.reliable) {
				collect.push_back(ReliableSubPacket(serializeMessages(pending.msgs), pending.seq));
			}
			pendingPackets.erase(iter);
		}
	}
}

ReliableSubPacket MessageQueue::createPacket()
{
	std::vector<std::unique_ptr<NetworkMessage>> sentMsgs;
	size_t maxSize = 1200;
	size_t size = 0;
	bool first = true;
	bool packetReliable = false;

	// Figure out what messages are going in this packet
	auto next = pendingMsgs.begin();
	for (auto iter = pendingMsgs.begin(); iter != pendingMsgs.end(); iter = next) {
		++next;
		auto& msg = *iter;

		// Check if this message is compatible
		auto& channel = channels[msg->channel];
		bool isReliable = channel.settings.reliable;
		bool isOrdered = channel.settings.ordered;
		if (first || isReliable == packetReliable) {
			// Check if the message fits
			size_t msgSize = (*iter)->getSerializedSize();
			size_t headerSize = 1 + (isOrdered ? 2 : 0) + (msgSize >= 64 ? 2 : 1);
			size_t totalSize = headerSize + msgSize;

			if (size + totalSize <= maxSize) {
				// It fits, so add it
				size += totalSize;

				sentMsgs.push_back(std::move(*iter));
				pendingMsgs.erase(iter);

				first = false;
				packetReliable = isReliable;
			}
		}
	}

	if (sentMsgs.empty()) {
		throw Exception("Was not able to fit any messages into packet!");
	}

	// Serialize
	auto data = serializeMessages(sentMsgs);

	// Track data in this packet
	int tag = nextPacketId++;
	auto& pendingData = pendingPackets[tag];
	pendingData.msgs = std::move(sentMsgs);
	pendingData.reliable = packetReliable;
	pendingData.timeSent = std::chrono::steady_clock::now();

	return ReliableSubPacket(std::move(data));
}

std::vector<gsl::byte> MessageQueue::serializeMessages(const std::vector<std::unique_ptr<NetworkMessage>>& msgs) const
{
	std::vector<gsl::byte> result;
	// TODO
	return result;
}
