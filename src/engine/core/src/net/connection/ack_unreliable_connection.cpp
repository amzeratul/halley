#include "halley/net/connection/ack_unreliable_connection.h"
#include "halley/net/connection/network_packet.h"
#include "halley/net/connection/network_service.h"
#include <chrono>
#include <utility>
#include <halley/utils/utils.h>
#include <halley/support/exception.h>

#include "halley/bytes/byte_serializer.h"
#include <halley/maths/random.h>
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

using namespace Halley;

#if 0
struct AckUnreliableHeader
{
	uint16_t sequence = 0xFFFF;
	uint16_t ack = 0xFFFF;
	uint32_t ackBits = 0xFFFFFFFF;

	void serialize(Serializer& s) const;
	void deserialize(Deserializer& s);
};

void AckUnreliableHeader::serialize(Serializer& s) const
{
	s << gsl::as_bytes(gsl::span<const AckUnreliableHeader>(this, 1));
}

void AckUnreliableHeader::deserialize(Deserializer& s)
{
	// Technically UB
	s >> gsl::as_writable_bytes(gsl::span<AckUnreliableHeader>(this, 1));
}

constexpr size_t BUFFER_SIZE = 1024;

AckUnreliableConnection::AckUnreliableConnection(std::shared_ptr<IConnection> parent)
	: parent(std::move(parent))
	, receivedSeqs(BUFFER_SIZE)
	, sentPackets(BUFFER_SIZE)
{
	lastSend = lastReceive = Clock::now();
}

void AckUnreliableConnection::close()
{
	parent->close();
}

ConnectionStatus AckUnreliableConnection::getStatus() const
{
	return parent->getStatus();
}

bool AckUnreliableConnection::isSupported(TransmissionType type) const
{
	return type == TransmissionType::Unreliable;
}

void AckUnreliableConnection::send(TransmissionType type, OutboundNetworkPacket packet)
{
	AckUnreliableSubPacket subPacket;
	subPacket.data.resize(packet.getSize());
	packet.copyTo(subPacket.data);
	subPacket.resends = false;
	subPacket.tag = -1;

	sendTagged(gsl::span<AckUnreliableSubPacket>(&subPacket, 1));
}

bool AckUnreliableConnection::receive(InboundNetworkPacket& packet)
{
	// Process all incoming
	try {
		InboundNetworkPacket tmp;
		while (parent->receive(tmp)) {
			lastReceive = Clock::now();
			processReceivedPacket(tmp);
		}
	} catch (std::exception& e) {
		std::cout << "Error receiving packets: " << e.what() << std::endl;
		close();
		return false;
	}

	if (!pendingPackets.empty()) {
		packet = std::move(pendingPackets.front());
		pendingPackets.pop_front();
		return true;
	}

	return false;
}

Vector<uint16_t> AckUnreliableConnection::sendTagged(gsl::span<const AckUnreliableSubPacket> subPackets)
{
	Vector<uint16_t> result;
	auto subPacketsLeft = subPackets;

	while (!subPacketsLeft.empty()) {
		std::array<gsl::byte, 16 * 1024> buffer;
		const auto dst = gsl::span<gsl::byte>(buffer);

		auto s = Serializer(dst, SerializerOptions(SerializerOptions::maxVersion));

		// Add header
		const auto seq = nextSequenceToSend++;
		AckUnreliableHeader header;
		header.sequence = seq;
		header.ack = highestReceived;
		header.ackBits = generateAckBits();
		s << header;

		auto& sent = sentPackets[seq % BUFFER_SIZE];
		sent = SentPacketData{};

		// Add subpackets
		bool first = true;
		while (!subPacketsLeft.empty()) {
			const auto& subPacket = subPacketsLeft.front();

			const size_t sizeNeeded = 2 + (subPacket.resends ? 2 : 0) + subPacket.data.size();
			const size_t sizeLeft = buffer.size() - s.getPosition();
			if (sizeNeeded > sizeLeft) {
				if (first) {
					throw Exception("Attempting to send packet that's too large for the network: " + String::prettySize(sizeNeeded), HalleyExceptions::Network);
				}
				break;
			}
			first = false;

			const uint16_t sizeAndResend = static_cast<uint16_t>(subPacket.data.size() << 1) | static_cast<uint16_t>(subPacket.resends ? 1 : 0);
			s << sizeAndResend;
			if (subPacket.resends) {
				s << subPacket.resendSeq;
			}
			s << gsl::span<const gsl::byte>(subPacket.data);

			sent.tags.push_back(subPacket.tag);

			if (subPacket.resends) {
				notifyResend(subPacket.resendSeq);
			}

			subPacketsLeft = subPacketsLeft.subspan(1);

			result.push_back(seq);
		}

		// Mark waiting
		sent.waiting = true;
		lastSend = sent.timestamp = Clock::now();

		// Send
		parent->send(TransmissionType::Unreliable, OutboundNetworkPacket(dst.subspan(0, s.getSize())));
		notifySend(header.sequence, s.getSize());
		earliestUnackedMsg = {};
	}

	return result;
}

void AckUnreliableConnection::sendAckPacketsIfNeeded()
{
	if (earliestUnackedMsg) {
		constexpr float maxAckTime = 0.02f;
		const float deltaTime = std::chrono::duration<float>(Clock::now() - earliestUnackedMsg.value()).count();
		if (deltaTime > maxAckTime) {
			// Send empty
			sendTagged(gsl::span<AckUnreliableSubPacket>());
		}
	}
}

void AckUnreliableConnection::processReceivedPacket(InboundNetworkPacket& packet)
{
	//Logger::logDev("Received packet with " + toString(packet.getSize()) + " bytes: " + Encode::encodeBase16(packet.getBytes()));
	auto s = Deserializer(packet.getBytes(), SerializerOptions(SerializerOptions::maxVersion));

	AckUnreliableHeader header;
	s >> header;

	processReceivedAcks(header.ack, header.ackBits);
	const uint16_t seq = header.sequence;

	if (onSeqReceived(seq, s.getBytesLeft() > 0)) {
		if (s.getBytesLeft() == 0) {
			notifyReceive(seq, 0, false);
		}

		while (s.getBytesLeft() > 0) {
			// Header
			uint16_t sizeAndResend = 0;
			s >> sizeAndResend;
			const uint16_t size = sizeAndResend >> 1;
			const bool resend = (sizeAndResend & 1) != 0;
			uint16_t resendOf = 0;
			if (resend) {
				s >> resendOf;
			}

			// Extract data
			std::array<char, 16 * 1024> buffer;
			if (size > buffer.size() || size > s.getBytesLeft()) {
				throw Exception("Unexpected sub-packet size: " + toString(size) + " bytes, " + toString(s.getBytesLeft()) + " bytes remaining.", HalleyExceptions::Network);
			}
			auto subPacketData = gsl::as_writable_bytes(gsl::span<char>(buffer).subspan(0, size));
			s >> subPacketData;
			
			if (!resend || onSeqReceived(resendOf, true)) {
				pendingPackets.emplace_back(subPacketData);
			}

			notifyReceive(seq, size, resend);
		}
	}
}

void AckUnreliableConnection::addAckListener(IAckUnreliableConnectionListener& listener)
{
	ackListeners.push_back(&listener);
}

void AckUnreliableConnection::removeAckListener(IAckUnreliableConnectionListener& listener)
{
	ackListeners.erase(std::find(ackListeners.begin(), ackListeners.end(), &listener));
}

void AckUnreliableConnection::processReceivedAcks(uint16_t ack, unsigned int ackBits)
{
	// If acking something too far back in the past, ignore it
	uint16_t diff = nextSequenceToSend - ack;
	if (diff > 512) {
		return;
	}

	startLatencyReport();
	for (int i = 32; --i >= 0; ) {
		if (ackBits & (1 << i)) {
			uint16_t seq = static_cast<uint16_t>(ack - (i + 1));
			onAckReceived(seq);
		}
	}
	onAckReceived(ack);
	endLatencyReport();
}

bool AckUnreliableConnection::onSeqReceived(uint16_t seq, bool hasSubPacket)
{
	const size_t bufferPos = size_t(seq) % BUFFER_SIZE;
	const uint16_t diff = seq - highestReceived;

	if (diff != 0 && diff < 0x8000) { // seq higher than highestReceived, with unsigned wrap-around
		if (diff > BUFFER_SIZE - 32) {
			// Ops, skipped too many packets!
			Logger::logError("Connection error: too many packets dropped.");
			close();
			return false;
		}

		// Clear all packets half-buffer seqs ago (since the last cleared one)
		for (size_t i = highestReceived % BUFFER_SIZE; i != bufferPos; i = (i + 1) % BUFFER_SIZE) {
			size_t idx = (i + BUFFER_SIZE / 2) % BUFFER_SIZE;
			receivedSeqs[idx] = 0;
		}

		highestReceived = seq;
	}

	if (receivedSeqs[bufferPos] != 0) {
		// Already received
		return false;
	} else {
		// Mark this packet as received
		receivedSeqs[bufferPos] = 1;
		if (hasSubPacket && !earliestUnackedMsg) {
			earliestUnackedMsg = Clock::now();
		}

		return true;
	}
}

void AckUnreliableConnection::onAckReceived(uint16_t sequence)
{
	auto& data = sentPackets[sequence % BUFFER_SIZE];
	if (data.waiting) {
		const float msgLag = std::chrono::duration<float>(Clock::now() - data.timestamp).count();

		data.waiting = false;
		for (int tag: data.tags) {
			for (const auto& listener: ackListeners) {
				listener->onPacketAcked(tag);
			}
		}
		reportLatency(msgLag);

		notifyAck(sequence);
	}
}

unsigned int AckUnreliableConnection::generateAckBits()
{
	unsigned int result = 0;
	
	for (size_t i = 0; i < 32; i++) {
		size_t bufferPos = ((highestReceived - 1 - i) + 0x10000) % BUFFER_SIZE;
		result |= static_cast<unsigned int>(1 & receivedSeqs[bufferPos]) << i;
	}

	return result;
}

float AckUnreliableConnection::getTimeSinceLastSend() const
{
	return std::chrono::duration<float>(Clock::now() - lastSend).count();
}

float AckUnreliableConnection::getTimeSinceLastReceive() const
{
	return std::chrono::duration<float>(Clock::now() - lastReceive).count();
}

void AckUnreliableConnection::setStatsListener(IAckUnreliableConnectionStatsListener* listener)
{
	statsListener = listener;
}

void AckUnreliableConnection::startLatencyReport()
{
	curLag = std::numeric_limits<float>::infinity();
}

void AckUnreliableConnection::reportLatency(float lastMeasuredLag)
{
	curLag = std::min(curLag, lastMeasuredLag);
}

void AckUnreliableConnection::endLatencyReport()
{
	if (fabs(lag) < 0.00001f) {
		lag = curLag;
	} else if (curLag < 30) {
		lag = lerp(lag, curLag, 0.2f);
	}
}

void AckUnreliableConnection::notifySend(uint16_t sequence, size_t size)
{
	if (statsListener) {
		statsListener->onPacketSent(sequence, size);
	}
}

void AckUnreliableConnection::notifyResend(uint16_t sequence)
{
	if (statsListener) {
		statsListener->onPacketResent(sequence);
	}
}

void AckUnreliableConnection::notifyAck(uint16_t sequence)
{
	if (statsListener) {
		statsListener->onPacketAcked(sequence);
	}
}

void AckUnreliableConnection::notifyReceive(uint16_t sequence, size_t size, bool resend)
{
	if (statsListener) {
		statsListener->onPacketReceived(sequence, size, resend);
	}
}
#endif

AckUnreliableConnectionV2::AckUnreliableConnectionV2(std::shared_ptr<IConnection> parent, INetworkServiceStatsListener& networkStatsListener)
    : parent(std::move(parent))
    , networkStatsListener(networkStatsListener)
{
    this->parent->setUnreliablePacketListener(this);

    maxPacketSize = this->parent->getMaxUnreliablePacketSize();

    inboundCache.resize_no_init(16 * maxPacketSize);

    for (auto &p: inbound.packets) {
        p.data.resize_no_init(maxPacketSize);
        p.seqIdx = 0xffff;
    }

    for (auto &p: outbound.packets) {
        p.data.resize_no_init(maxPacketSize);
        p.seqIdx = 0xffff;
    }
}

void AckUnreliableConnectionV2::close()
{
    parent->close();
}

bool AckUnreliableConnectionV2::isSupported(TransmissionType type) const
{
    return type == TransmissionType::Unreliable;
}

ConnectionStatus AckUnreliableConnectionV2::getStatus() const
{
    return parent->getStatus();
}

void AckUnreliableConnectionV2::send(TransmissionType type, OutboundNetworkPacket packet)
{
    Expects(type == TransmissionType::Unreliable);

    auto status = parent->getStatus();
    if (status != ConnectionStatus::Connected) {
        return;
    }

    size_t size = packet.getSize();
    size_t maxSize = maxPacketSize - headerSize;

    if (maxSize >= size) {
        auto& slot = outbound.packets[outbound.curPacketIdx];

        if (slot.seqIdx < 0x8000) {
            Logger::logError("Outbound packet queue is full");
            close();
            return;
        }

        slot.dataSize = packet.copyTo(slot.data.byte_span().subspan(headerSize));
        slot.seqIdx = outbound.curSeqIdx;
        slot.subIdx = 0;

        doSend(slot, outbound.curPacketIdx);

        outbound.curPacketIdx = (outbound.curPacketIdx + 1) % 256;
    } else {
        size_t numSubPackets = size / maxSize;
        if (size % maxSize != 0) {
            numSubPackets++;
        }

        if (numSubPackets > 16) {
            throw Exception("Packet size too large: " + toString(packet.getSize()) + " bytes", HalleyExceptions::Network);
        }

        auto packetData = packet.getBytes();

        for (size_t i = 0; i < numSubPackets; i++) {
            auto& slot = outbound.packets[outbound.curPacketIdx];

            if (slot.seqIdx < 0x8000) {
                Logger::logError("Outbound packet queue is full");
                close();
                return;
            }

            slot.dataSize = std::min(packetData.size(), maxSize);
            memcpy(slot.data.data() + headerSize, packetData.data(), slot.dataSize);

            packetData = packetData.subspan(slot.dataSize);

            slot.seqIdx = outbound.curSeqIdx;
            slot.subIdx = uint8_t(i) | uint8_t(numSubPackets << 4);

            doSend(slot, outbound.curPacketIdx);

            outbound.curPacketIdx = (outbound.curPacketIdx + 1) % 256;
        }

        Expects(packetData.empty());
    }

    networkStatsListener.onSendData(size, 1);

    if (statsListener) {
        statsListener->onPacketSent(outbound.curSeqIdx, size);
    }

    outbound.curSeqIdx = (outbound.curSeqIdx + 1) % 0x8000;
}

void AckUnreliableConnectionV2::doSend(SubPacket& packet, int packetIdx)
{
    Expects(packetIdx >= 0 && packetIdx < 256);

    uint8_t* header = packet.data.data();

    header[0] = headerSignature[0];
    header[1] = headerSignature[1];
    header[2] = headerSignature[2];
    header[3] = headerSignature[3];

    header[4] = packet.seqIdx >> 8;
    header[5] = packet.seqIdx & 0xff;
    header[6] = packet.subIdx;
    header[7] = packetIdx & 0xff;

    // bytes 8 to 13 are unused right now

    // bytes 14+15 are reserved!
    header[14] = 0;
    header[15] = 0;

	packet.timestamp = Clock::now();

	doSendUnreliablePacket(packet.data.byte_span().subspan(0, headerSize + packet.dataSize), packet.seqIdx);
}

void AckUnreliableConnectionV2::doSendUnreliablePacket(gsl::span<const gsl::byte> packet, uint16_t seqIdx)
{
#ifdef DEV_BUILD
	if (simulatePacketLoss > 0.0f && seqIdx != 0xffff) {
		if (Random::getGlobal().getFloat(0.0f, 1.0f) < simulatePacketLoss) {
			Logger::logDev("[x] lose " + toString(seqIdx));
			return;
		}
	}
#endif

	parent->sendUnreliablePacket(packet);
}

bool AckUnreliableConnectionV2::receive(InboundNetworkPacket& packet)
{
    auto status = parent->getStatus();
    if (status != ConnectionStatus::Connected) {
        return false;
    }

	// Need to send ack packets first, code below invalidates the sequence indices.
	doSendAckPackets();

    auto& slot = inbound.packets[inbound.curPacketIdx];

    if (slot.seqIdx < 0x8000) {
        if (slot.seqIdx != inbound.curSeqIdx) {
            throw Exception("Unexpected packet sequence index", HalleyExceptions::Network);
        }

        if (slot.subIdx == 0) {
            packet = InboundNetworkPacket(slot.data.byte_span().subspan(0, slot.dataSize));

            slot.seqIdx = 0xffff;

            networkStatsListener.onReceiveData(slot.dataSize, 1);

            if (statsListener) {
                statsListener->onPacketReceived(inbound.curSeqIdx, slot.dataSize, false);
            }

        	//Logger::logDev("[#] complete " + toString(inbound.curSeqIdx));

        	inbound.curPacketIdx = (inbound.curPacketIdx + 1) % 256;
            inbound.curSeqIdx = (inbound.curSeqIdx + 1) % 0x8000;

            return true;
        } else {
            // Check if all sub-packets have arrived.
            int numSubPackets = slot.subIdx >> 4;
            Expects(numSubPackets > 1 && (slot.subIdx & 15) == 0);

            bool isComplete = true;
            for (size_t i = 1; isComplete && i < numSubPackets; i++) {
                const auto& sub = inbound.packets[(inbound.curPacketIdx + i) % 256];
                isComplete &= sub.seqIdx == slot.seqIdx;
                isComplete &= (sub.subIdx & 15) == i;
                isComplete &= (sub.subIdx >> 4) == numSubPackets;
            }

            if (isComplete) {
                size_t totalSize = 0;

                for (size_t i = 0; i < numSubPackets; i++) {
                    auto& sub = inbound.packets[(inbound.curPacketIdx + i) % 256];

                    memcpy(inboundCache.data() + totalSize, sub.data.data(), sub.dataSize);
                    totalSize += sub.dataSize;

                    sub.seqIdx = 0xffff;
                }

                packet = InboundNetworkPacket(inboundCache.byte_span().subspan(0, totalSize));

                networkStatsListener.onReceiveData(totalSize, numSubPackets);

                if (statsListener) {
                    statsListener->onPacketReceived(inbound.curSeqIdx, totalSize, false);
                }

            	//Logger::logDev("[#] complete " + toString(inbound.curSeqIdx));

            	inbound.curPacketIdx = (inbound.curPacketIdx + numSubPackets) % 256;
                inbound.curSeqIdx = (inbound.curSeqIdx + 1) % 0x8000;

                return true;
            }
        }
    }

	resendUnAckPackets(std::clamp(lag * 1.5f, 0.025f, 1.0f));

    return false;
}

void AckUnreliableConnectionV2::onSend(gsl::span<const gsl::byte> packet)
{
}

void AckUnreliableConnectionV2::onReceive(gsl::span<const gsl::byte> packet)
{
    size_t packetSize = packet.size();
    auto* header = (const uint8_t*) packet.data();

    if (packetSize < 6 || memcmp(header, headerSignature, 4) != 0) {
        return; // too small, or wrong signature
    }

    uint16_t seqIdx = uint16_t(header[4] << 8) | header[5];

    if (seqIdx == 0xffff) {
    	// This is an ACK packet.
        onAckPacketsReceive(packet.subspan(6));
        return;
    }

    uint8_t subIdx = header[6];
    int packetIdx = header[7];

    size_t connIdLen = 0;
    /*size_t connIdLen = 1;
    if ((header[15] & 0x80) != 0) {
        connIdLen = 2;
    }*/

	size_t dataSize = packetSize - headerSize + connIdLen;

    auto& slot = inbound.packets[packetIdx];

	if (slot.seqIdx == seqIdx) {
		// Already got this packet.
		if (slot.subIdx != subIdx || slot.dataSize != dataSize) {
	        throw Exception("Rcv resent packet, but data mismatch", HalleyExceptions::Network);
		}
		if (statsListener) {
			statsListener->onPacketReceived(seqIdx, dataSize, true);
		}
		Logger::logDev("[x] drop " + toString(seqIdx));
		return;
	}

	if (isExpiredSeqIndex(inbound, seqIdx)) {
		return;
	}

    if (slot.seqIdx < 0x8000) {
        throw Exception("Too many inbound packets", HalleyExceptions::Network);
    }

	//Logger::logDev("[ ] recv " + toString(seqIdx));

    slot.dataSize = dataSize;
    memcpy(slot.data.data(), packet.data() + headerSize - connIdLen, dataSize);

    slot.seqIdx = seqIdx;
    slot.subIdx = subIdx;

    Expects(numOutboundAckPackets < 256);
    outboundAckPackets[numOutboundAckPackets++] = packetIdx;
}

void AckUnreliableConnectionV2::doSendAckPackets()
{
    if (numOutboundAckPackets == 0) {
        return;
    }

    std::array<uint8_t, 6 + 256 * 3> packet = {};

    uint8_t* msg = packet.data();

    msg[0] = headerSignature[0];
    msg[1] = headerSignature[1];
    msg[2] = headerSignature[2];
    msg[3] = headerSignature[3];

    msg[4] = 0xff;
    msg[5] = 0xff;

    for (int i = 0; i < numOutboundAckPackets; i++) {
    	uint8_t packetIdx = outboundAckPackets[i];

    	auto& slot = inbound.packets[packetIdx];

        msg[6 + 3 * i] = packetIdx;
        msg[6 + 3 * i + 1] = slot.seqIdx >> 8;
        msg[6 + 3 * i + 2] = slot.seqIdx & 0xff;

    	//Logger::logDev("[ ] ack " + toString(slot.seqIdx));
    }

    // NOTE: this isn't sent reliably either
	doSendUnreliablePacket(gsl::span<const gsl::byte>(reinterpret_cast<gsl::byte *>(packet.data()), 6 + 3 * numOutboundAckPackets), 0xffff);

	numOutboundAckPackets = 0;

#if 0
	// Walk forward the "first packet in use" index.
	while (inbound.firstPacketIdx != inbound.curPacketIdx) {
		auto slot = &inbound.packets[inbound.firstPacketIdx];
		if (slot->seqIdx != 0xffff) {
			break;
		}
		inbound.firstPacketIdx = (inbound.firstPacketIdx + 1) % 256;
	}
#endif
}

void AckUnreliableConnectionV2::onAckPacketsReceive(gsl::span<const gsl::byte> data)
{
    auto ackData = reinterpret_cast<const uint8_t *>(data.data());
    size_t size = data.size();

	Ensures(size > 0 && (size % 3) == 0);

	float avgLatencySum = 0.f;
	size_t avgLatencyCount = 0;
	const Clock::time_point now = Clock::now();

    for (size_t i = 0; i < size; i += 3) {
    	uint8_t packetIdx = ackData[i];
    	uint16_t seqIdx = uint16_t(ackData[i + 1] << 8) | ackData[i + 2];

        auto slot = &outbound.packets[packetIdx];

    	if (slot->seqIdx != 0xffff) {
    		if (slot->seqIdx == seqIdx) {
    			if (statsListener) {
    				statsListener->onPacketAcked(slot->seqIdx);
    			}

    			//Logger::logDev("[ ] rcv ack " + toString(seqIdx));

    			slot->seqIdx = 0xffff;

    			avgLatencySum += std::chrono::duration<float>(now - slot->timestamp).count();
    			avgLatencyCount++;
    		} else {
	    		Logger::logDev("rcv mismatch ACK for slot " + toString((int) packetIdx) + ", seqIdx " + toString(slot->seqIdx) + ", remote seqIdx " + toString(seqIdx));
    		}
    	} else {
    		if (!isExpiredSeqIndex(outbound, seqIdx)) {
    			// Resent packets can be ack'd more than once.
    			Logger::logDev("rcv ACK for empty slot " + toString((int) packetIdx) + ", remote seqIdx " + toString(seqIdx));
    		}
    	}
    }

	// Now that this ACK packet has been processed, walk forward the "first
	// packet in use" index.
	while (outbound.firstPacketIdx != outbound.curPacketIdx) {
		auto slot = &outbound.packets[outbound.firstPacketIdx];
		if (slot->seqIdx != 0xffff) {
			break;
		}
		outbound.firstPacketIdx = (outbound.firstPacketIdx + 1) % 256;
	}

	// Re-send packets which are not yet acknowledged.
	// Ignore packet age, we just re-send everything that still lingers at start of the
	// outbound queue.
	//resendUnAckPackets(0.0f);

	// Update latency
	if (avgLatencyCount > 0) {
		avgLatencySum /= static_cast<float>(avgLatencyCount);
		lag = lerp(lag, avgLatencySum, 0.2f);
	}
}

void AckUnreliableConnectionV2::resendUnAckPackets(float minResendTimeDiff)
{
	const Clock::time_point now = Clock::now();

	// This re-sends packets which are not yet acknowledged.
	// Stops at the first "gap", which would be a packet which we already got
	// an ACK for.
	int idx = outbound.firstPacketIdx;
	while (idx != outbound.curPacketIdx) {
		auto slot = &outbound.packets[idx];

		if (slot->seqIdx == 0xffff) {
			break; // stop at the first index not in use
		}

		float timeSinceSent = std::chrono::duration<float>(now - slot->timestamp).count();
		if (timeSinceSent < minResendTimeDiff) {
			break; // stop at first packet queued up too recently
		}

		doSendUnreliablePacket(slot->data.byte_span().subspan(0, headerSize + slot->dataSize), slot->seqIdx);

		if (statsListener) {
			statsListener->onPacketResent(slot->seqIdx);
		}

		// Update timestamp, or this will spam each frame.
		slot->timestamp = now;

		idx = (idx + 1) % 256;
	}
}

float AckUnreliableConnectionV2::getLatency() const
{
    return lag;
}

void AckUnreliableConnectionV2::setStatsListener(IAckUnreliableConnectionStatsListener* listener)
{
    statsListener = listener;
}

bool AckUnreliableConnectionV2::isExpiredSeqIndex(const InOutQueue& queue, uint16_t seqIdx)
{
	// Sequence indices wrap, so we can't just plain compare.
	if (seqIdx > queue.curSeqIdx) {
		return (static_cast<size_t>(queue.curSeqIdx) + 4096) < seqIdx;
	}

	return seqIdx < queue.curSeqIdx;
}
