#include "message_queue.h"
#include "reliable_connection.h"
#include <halley/support/exception.h>
#include <network_packet.h>
#include <iostream>

using namespace Halley;

ChannelSettings::ChannelSettings(bool reliable, bool ordered, bool keepLastSent)
	: reliable(reliable)
	, ordered(ordered)
	, keepLastSent(keepLastSent)
{}

void MessageQueue::Channel::getReadyMessages(std::vector<std::unique_ptr<NetworkMessage>>& out)
{
	if (settings.ordered) {
		if (settings.reliable) {
			bool trying = true;
			// Oh my god, this is horrible
			while (trying && !receiveQueue.empty()) {
				trying = false;
				for (size_t i = 0; i < receiveQueue.size(); ++i) {
					auto& m = receiveQueue[i];
					unsigned short expected = lastReceivedSeq + 1;
					if (m->seq == expected) {
						trying = true;
						out.push_back(std::move(m));
						if (receiveQueue.size() > 1) {
							std::swap(receiveQueue[i], receiveQueue[receiveQueue.size() - 1]);
						}
						receiveQueue.pop_back();
						lastReceivedSeq++;
						break;
					}
				}
			}
		} else {
			unsigned short bestDist = 0;
			size_t fail = size_t(-1);
			size_t best = fail;
			// Look for the highest seq message, as long as it's above lastReceived
			for (size_t i = 0; i < receiveQueue.size(); ++i) {
				auto& m = receiveQueue[i];
				unsigned short dist = m->seq - lastReceivedSeq;
				if (dist < 0x7FFF) {
					if (dist > bestDist) {
						bestDist = dist;
						best = i;
					}
				}
			}
			if (best != fail) {
				lastReceivedSeq = receiveQueue[best]->seq;
				out.push_back(std::move(receiveQueue[best]));
			}
			receiveQueue.clear();
		}
	} else {
		for (auto& m: receiveQueue) {
			out.emplace_back(std::move(m));
		}
		receiveQueue.clear();
	}
}

MessageQueue::MessageQueue(std::shared_ptr<ReliableConnection> connection)
	: connection(connection)
	, channels(32)
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
	Expects(channel >= 0);
	Expects(channel < 32);

	if (channels[channel].initialized) {
		throw Exception("Channel " + toString(channel) + " already set");
	}

	auto& c = channels[channel];
	c.settings = settings;
	c.initialized = true;
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

std::vector<std::unique_ptr<NetworkMessage>> MessageQueue::receiveAll()
{
	if (connection->getStatus() == ConnectionStatus::OPEN) {
		receiveMessages();
	}

	std::vector<std::unique_ptr<NetworkMessage>> result;
	for (auto& c: channels) {
		c.getReadyMessages(result);
	}
	return result;
}

void MessageQueue::enqueue(std::unique_ptr<NetworkMessage> msg, int channelNumber)
{
	Expects(channelNumber >= 0);
	Expects(channelNumber < 32);

	if (!channels[channelNumber].initialized) {
		throw Exception("Channel " + toString(channelNumber) + " has not been set up");
	}
	auto& channel = channels[channelNumber];

	msg->channel = channelNumber;
	msg->seq = ++channel.lastSentSeq;

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
		if (elapsed > 0.1f && elapsed > connection->getLatency() * 3.0f) {
			// Re-send if it's reliable
			if (pending.reliable) {
				collect.push_back(makeTaggedPacket(pending.msgs, pending.size, true, pending.seq));
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
			int msgType = getMessageType(**iter);
			size_t headerSize = 1 + (isOrdered ? 2 : 0) + (msgSize >= 128 ? 2 : 1) + (msgType >= 128 ? 2 : 1);
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

	return makeTaggedPacket(sentMsgs, size);
}

ReliableSubPacket MessageQueue::makeTaggedPacket(std::vector<std::unique_ptr<NetworkMessage>>& msgs, size_t size, bool resends, unsigned short resendSeq)
{
	bool reliable = !msgs.empty() && channels[msgs[0]->channel].settings.reliable;

	auto data = serializeMessages(msgs, size);

	int tag = nextPacketId++;
	auto& pendingData = pendingPackets[tag];
	pendingData.msgs = std::move(msgs);
	pendingData.size = size;
	pendingData.reliable = reliable;
	pendingData.timeSent = std::chrono::steady_clock::now();

	auto result = ReliableSubPacket(std::move(data));
	result.tag = tag;
	result.resends = resends;
	result.resendSeq = resendSeq;
	return result;
}

std::vector<gsl::byte> MessageQueue::serializeMessages(const std::vector<std::unique_ptr<NetworkMessage>>& msgs, size_t size) const
{
	std::vector<gsl::byte> result(size);
	size_t pos = 0;
	
	for (auto& msg: msgs) {
		size_t msgSize = msg->getSerializedSize();
		int msgType = getMessageType(*msg);
		char channelN = msg->channel;

		auto& channel = channels[channelN];
		bool isOrdered = channel.settings.ordered;

		// Write header
		memcpy(&result[pos], &channelN, 1);
		pos += 1;
		if (isOrdered) {
			unsigned short sequence = static_cast<unsigned short>(msg->seq);
			memcpy(&result[pos], &sequence, 2);
			pos += 2;
		}
		if (msgSize >= 128) {
			std::array<unsigned char, 2> bytes;
			bytes[0] = static_cast<unsigned char>(msgSize >> 8) | 0x80;
			bytes[1] = static_cast<unsigned char>(msgSize & 0xFF);
			memcpy(&result[pos], bytes.data(), 2);
			pos += 2;
		} else {
			unsigned char byte = msgSize & 0x7F;
			memcpy(&result[pos], &byte, 1);
			pos += 1;
		}
		if (msgType >= 128) {
			std::array<unsigned char, 2> bytes;
			bytes[0] = static_cast<unsigned char>(msgType >> 8) | 0x80;
			bytes[1] = static_cast<unsigned char>(msgType & 0xFF);
			memcpy(&result[pos], bytes.data(), 2);
			pos += 2;
		}
		else {
			unsigned char byte = msgType & 0x7F;
			memcpy(&result[pos], &byte, 1);
			pos += 1;
		}

		// Write message
		msg->serializeTo(gsl::span<gsl::byte>(result).subspan(pos, msgSize));
		pos += msgSize;
	}

	return result;
}

void MessageQueue::receiveMessages()
{
	try {
		InboundNetworkPacket packet;
		while (connection->receive(packet)) {
			auto data = packet.getBytes();

			while (data.size() > 0) {
				// Read channel
				char channelN;
				memcpy(&channelN, data.data(), 1);
				data = data.subspan(1);
				if (channelN < 0 || channelN >= 32) {
					throw Exception("Received invalid channel");
				}
				auto& channel = channels[channelN];

				// Read sequence
				unsigned short sequence = 0;
				if (channel.settings.ordered) {
					if (data.size() < 2) {
						throw Exception("Missing sequence data");
					}
					memcpy(&sequence, data.data(), 2);
					data = data.subspan(2);
				}

				// Read size
				size_t size;
				unsigned char b0;
				if (data.size() < 1) {
					throw Exception("Missing size data");
				}
				memcpy(&b0, data.data(), 1);
				data = data.subspan(1);
				if (b0 & 0x80) {
					if (data.size() < 1) {
						throw Exception("Missing size data");
					}
					unsigned char b1;
					memcpy(&b1, data.data(), 1);
					data = data.subspan(1);
					size = (static_cast<unsigned short>(b0 & 0x7F) << 8) | static_cast<unsigned short>(b1);
				} else {
					size = b0;
				}

				// Read message type
				unsigned short msgType;
				if (data.size() < 1) {
					throw Exception("Missing msgType data");
				}
				memcpy(&b0, data.data(), 1);
				data = data.subspan(1);
				if (b0 & 0x80) {
					if (data.size() < 1) {
						throw Exception("Missing msgType data");
					}
					unsigned char b1;
					memcpy(&b1, data.data(), 1);
					data = data.subspan(1);
					msgType = (static_cast<unsigned short>(b0 & 0x7F) << 8) | static_cast<unsigned short>(b1);
				} else {
					msgType = b0;
				}

				// Read message
				if (data.size() < signed(size)) {
					throw Exception("Message does not contain enough data");
				}
				channel.receiveQueue.emplace_back(deserializeMessage(data.subspan(0, size), msgType, sequence));
				data = data.subspan(size);
			}
		}
	} catch (std::exception& e) {
		std::cout << "Error receiving messages: " << e.what() << std::endl;
		connection->close();
	} catch (...) {
		connection->close();
	}
}

std::unique_ptr<NetworkMessage> MessageQueue::deserializeMessage(gsl::span<const gsl::byte> data, unsigned short msgType, unsigned short seq)
{
	auto msg = factories.at(msgType)->create();
	msg->deserializeFrom(data);
	msg->seq = seq;
	return std::move(msg);
}
