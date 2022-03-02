#include "halley/net/connection/message_queue_udp.h"
#include <iostream>
#include <utility>
using namespace Halley;

void MessageQueueUDP::Channel::getReadyMessages(Vector<InboundNetworkPacket>& out)
{
	if (settings.ordered) {
		if (settings.reliable) {
			bool trying = true;
			// Oh my god, this is horrible
			while (trying && !receiveQueue.empty()) {
				trying = false;
				for (size_t i = 0; i < receiveQueue.size(); ++i) {
					auto& m = receiveQueue[i];
					const uint16_t expected = lastReceivedSeq + 1;
					if (m.seq == expected) {
						trying = true;
						out.push_back(std::move(m.packet));
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
			uint16_t bestDist = 0;
			const size_t fail = std::numeric_limits<size_t>::max();
			size_t best = fail;
			// Look for the highest seq message, as long as it's above lastReceived
			for (size_t i = 0; i < receiveQueue.size(); ++i) {
				auto& m = receiveQueue[i];
				const uint16_t dist = m.seq - lastReceivedSeq;
				if (dist < 0x7FFF) {
					if (dist > bestDist) {
						bestDist = dist;
						best = i;
					}
				}
			}
			if (best != fail) {
				lastReceivedSeq = receiveQueue[best].seq;
				out.push_back(std::move(receiveQueue[best].packet));
			}
			receiveQueue.clear();
		}
	} else {
		for (auto& m: receiveQueue) {
			out.emplace_back(std::move(m.packet));
		}
		receiveQueue.clear();
	}
}

MessageQueueUDP::MessageQueueUDP(std::shared_ptr<AckUnreliableConnection> conn)
	: connection(std::move(conn))
	, channels(32)
{
	Expects(connection != nullptr);
	connection->addAckListener(*this);
}

MessageQueueUDP::~MessageQueueUDP()
{
	connection->removeAckListener(*this);
}

void MessageQueueUDP::setChannel(uint8_t channel, ChannelSettings settings)
{
	if (channels.size() <= static_cast<size_t>(channel)) {
		channels.resize(static_cast<size_t>(channel) + 1);
	}

	if (channels[channel].initialized) {
		throw Exception("Channel " + toString(channel) + " already set", HalleyExceptions::Network);
	}

	auto& c = channels[channel];
	c.settings = settings;
	c.initialized = true;
}

Vector<gsl::byte> MessageQueueUDP::serializeMessages(const Vector<Outbound>& msgs, size_t size) const
{
	Vector<gsl::byte> result(size);
	
	for (auto& msg: msgs) {
		const uint8_t channelN = msg.channel;
		const auto& channel = channels[channelN];
		const bool isOrdered = channel.settings.ordered;

		auto s = Serializer(result, SerializerOptions(SerializerOptions::maxVersion));
		s << channelN;
		if (isOrdered) {
			s << msg.seq;
		}
		s << msg.packet.getBytes();
	}

	return result;
}

void MessageQueueUDP::receiveMessages()
{
	try {
		InboundNetworkPacket packet;
		while (connection->receive(packet)) {
			auto s = Deserializer(packet.getBytes(), SerializerOptions(SerializerOptions::maxVersion));

			while (s.getBytesLeft() > 0) {
				uint8_t channelN = 0;
				uint16_t sequence = 0;

				s >> channelN;
				auto& channel = channels.at(channelN);
				if (channel.settings.ordered) {
					s >> sequence;
				}

				Bytes msgData;
				s >> msgData;

				// Read message
				channel.receiveQueue.emplace_back(Inbound{ InboundNetworkPacket(gsl::as_bytes(gsl::span<const Byte>(msgData))), sequence, channelN });
			}
		}
	} catch (std::exception& e) {
		std::cout << "Error receiving messages: " << e.what() << std::endl;
		connection->close();
	} catch (...) {
		connection->close();
	}
}

Vector<InboundNetworkPacket> MessageQueueUDP::receivePackets()
{
	if (connection->getStatus() == ConnectionStatus::Connected) {
		receiveMessages();
	}

	Vector<InboundNetworkPacket> result;
	for (auto& c: channels) {
		c.getReadyMessages(result);
	}
	return result;
}

void MessageQueueUDP::enqueue(OutboundNetworkPacket packet, uint8_t channelNumber)
{
	Expects(channelNumber >= 0);
	Expects(channelNumber < 32);

	if (!channels[channelNumber].initialized) {
		throw Exception("Channel " + toString(channelNumber) + " has not been set up", HalleyExceptions::Network);
	}
	auto& channel = channels[channelNumber];

	outboundQueued.emplace_back(Outbound{ std::move(packet), ++channel.lastSentSeq, channelNumber });
}

void MessageQueueUDP::sendAll()
{
	//int firstTag = nextPacketId;
	Vector<AckUnreliableSubPacket> toSend;

	// Add packets which need to be re-sent
	checkReSend(toSend);

	// Create packets of pending messages
	while (!outboundQueued.empty()) {
		toSend.emplace_back(createPacket());
	}

	// Send and update sequences
	connection->sendTagged(toSend);
	for (auto& packet: toSend) {
		pendingPackets[packet.tag].seq = packet.seq;
	}
}

void MessageQueueUDP::onPacketAcked(int tag)
{
	auto i = pendingPackets.find(tag);
	if (i != pendingPackets.end()) {
		auto& packet = i->second;

		for (auto& m : packet.msgs) {
			auto& channel = channels[m.channel];
			if (m.seq - channel.lastAckSeq < 0x7FFFFFFF) {
				channel.lastAckSeq = m.seq;
				if (channel.settings.keepLastSent) {
					//channel.lastAck = std::move(m.packet);
					// TODO
				}
			}
		}

		// Remove pending
		pendingPackets.erase(tag);
	}
}

void MessageQueueUDP::checkReSend(Vector<AckUnreliableSubPacket>& collect)
{
	auto next = pendingPackets.begin();
	for (auto iter = pendingPackets.begin(); iter != pendingPackets.end(); iter = next) {
		++next;
		auto& pending = iter->second;

		// Check how long it's been waiting
		const float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - pending.timeSent).count();
		if (elapsed > 0.1f && elapsed > connection->getLatency() * 3.0f) {
			// Re-send if it's reliable
			if (pending.reliable) {
				collect.push_back(makeTaggedPacket(pending.msgs, pending.size, true, pending.seq));
			}
			pendingPackets.erase(iter);
		}
	}
}

AckUnreliableSubPacket MessageQueueUDP::createPacket()
{
	Vector<Outbound> sentMsgs;
	const size_t maxSize = 1300;
	size_t size = 0;
	bool first = true;
	bool packetReliable = false;

	// Figure out what messages are going in this packet
	auto next = outboundQueued.begin();
	for (auto iter = outboundQueued.begin(); iter != outboundQueued.end(); iter = next) {
		++next;
		const auto& msg = *iter;

		// Check if this message is compatible
		const auto& channel = channels[msg.channel];
		const bool isReliable = channel.settings.reliable;
		const bool isOrdered = channel.settings.ordered;
		if (first || isReliable == packetReliable) {
			// Check if the message fits
			const size_t msgSize = (*iter).packet.getSize();

			if (size + msgSize <= maxSize) {
				// It fits, so add it
				size += msgSize;

				sentMsgs.push_back(std::move(*iter));
				outboundQueued.erase(iter);

				first = false;
				packetReliable = isReliable;
			}
		}
	}

	if (sentMsgs.empty()) {
		throw Exception("Was not able to fit any messages into packet!", HalleyExceptions::Network);
	}

	return makeTaggedPacket(sentMsgs, size);
}

AckUnreliableSubPacket MessageQueueUDP::makeTaggedPacket(Vector<Outbound>& msgs, size_t size, bool resends, uint16_t resendSeq)
{
	const bool reliable = !msgs.empty() && channels[msgs[0].channel].settings.reliable;

	auto data = serializeMessages(msgs, size);

	const int tag = nextPacketId++;
	auto& pendingData = pendingPackets[tag];
	pendingData.msgs = std::move(msgs);
	pendingData.size = size;
	pendingData.reliable = reliable;
	pendingData.timeSent = std::chrono::steady_clock::now();

	auto result = AckUnreliableSubPacket(std::move(data));
	result.tag = tag;
	result.resends = resends;
	result.resendSeq = resendSeq;
	return result;
}
