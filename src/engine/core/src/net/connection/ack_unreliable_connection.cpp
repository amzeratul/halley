#include "halley/net/connection/ack_unreliable_connection.h"
#include "halley/net/connection/network_packet.h"
#include "halley/net/connection/network_service.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/maths/random.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

#define SIMULATE_PLATFORM_ACK 0

using namespace Halley;

AckUnreliableConnection::AckUnreliableConnection(std::shared_ptr<IConnection> parent, INetworkServiceStatsListener& networkStatsListener)
    : parent(std::move(parent))
    , networkStatsListener(networkStatsListener)
{
    this->parent->setUnreliablePacketListener(this);

    totalMaxPacketSize = this->parent->getMaxUnreliablePacketSize();
	realMaxPacketSize = 0; // this is only set after the connection has been established

    inboundCache.resize_no_init(16 * totalMaxPacketSize);

    for (auto& p: inbound.packets) {
        p.data.resize(totalMaxPacketSize);
        p.seqIdx = 0xffff;
    }

    for (auto& p: outbound.packets) {
        p.data.resize(totalMaxPacketSize);
        p.seqIdx = 0xffff;
    	memcpy(p.data.data(), headerSignature2, sizeof(headerSignature2));
    	memset(p.data.data() + sizeof(headerSignature2), 0, totalMaxPacketSize - sizeof(headerSignature2));
    }

	platformSupportsAck = this->parent->doesInternalPacketAck();
}

void AckUnreliableConnection::close()
{
    parent->close();
}

void AckUnreliableConnection::close(const std::optional<String>& reason)
{
	if (reason) {
		Logger::logError("Connection closed, " + reason.value());
	}

	close();
}

bool AckUnreliableConnection::isSupported(TransmissionType type) const
{
    return type == TransmissionType::Unreliable;
}

ConnectionStatus AckUnreliableConnection::getStatus() const
{
    return parent->getStatus();
}

void AckUnreliableConnection::send(TransmissionType type, OutboundNetworkPacket packet) {
	HalleyAssertDev(type == TransmissionType::Unreliable);

	auto status = parent->getStatus();
	if (status != ConnectionStatus::Connected) {
		return;
	}

	if (realMaxPacketSize == 0) {
		// Ask connection about the actual size we can use.
		realMaxPacketSize = parent->getRealMaxUnreliablePacketSize();
		HalleyAssertDebug(realMaxPacketSize <= totalMaxPacketSize);
		realMaxPacketSize = std::min(realMaxPacketSize, totalMaxPacketSize);
	}

	doSend(packet.getBytes());
}

void AckUnreliableConnection::doSend(gsl::span<const std::byte> packet)
{
	HalleyAssertDev(realMaxPacketSize > headerSize);

    const size_t size = packet.size();
    const size_t maxSize = realMaxPacketSize - headerSize;

    if (maxSize >= size) {
    	if (!checkOutboundQueue(1)) {
    		close("outbound packet queue is full");
    		return;
    	}

        auto& slot = outbound.packets[outbound.curPacketIdx];
        HalleyAssertDev(slot.seqIdx == 0xffff);

	    networkStatsListener.onSendData(size, 1);

    	slot.dataSize = size;
        memcpy(slot.data.data() + headerSize, packet.data(), size);
        slot.seqIdx = outbound.seqIndex.toSlot();
        slot.subIdx = 0;
    	slot.parity = outbound.seqIndex.parity();
    	slot.resend = 0;

        doSend(slot, outbound.curPacketIdx);

        outbound.curPacketIdx = (outbound.curPacketIdx + 1) % maxPacketQueueSize;
    } else {
        size_t numSubPackets = size / maxSize;
        if (size % maxSize != 0) {
            numSubPackets++;
        }

    	HalleyAssertDev(numSubPackets > 1);

        if (numSubPackets > 16) {
            throw Exception("Packet size too large: " + toString(size) + " bytes", HalleyExceptions::Network);
        }

    	if (!checkOutboundQueue(static_cast<int>(numSubPackets))) {
    		close("outbound packet queue is full");
    		return;
    	}

        for (size_t i = 0; i < numSubPackets; i++) {
            auto& slot = outbound.packets[outbound.curPacketIdx];
        	HalleyAssertDev(slot.seqIdx == 0xffff);

            slot.dataSize = std::min(packet.size(), maxSize);
            memcpy(slot.data.data() + headerSize, packet.data(), slot.dataSize);

            packet = packet.subspan(slot.dataSize);

            slot.seqIdx = outbound.seqIndex.toSlot();
            slot.subIdx = static_cast<uint8_t>(i) | static_cast<uint8_t>((numSubPackets - 1) << 4);
        	slot.parity = outbound.seqIndex.parity();
        	slot.resend = 0;

            doSend(slot, outbound.curPacketIdx);

            outbound.curPacketIdx = (outbound.curPacketIdx + 1) % maxPacketQueueSize;
        }

    	networkStatsListener.onSendData(size, numSubPackets);

        HalleyAssertDev(packet.empty());
    }

    if (statsListener) {
        statsListener->onPacketSent(outbound.seqIndex.toSlot(), size);
    }

    outbound.seqIndex.update();
}

void AckUnreliableConnection::doSend(SubPacket& packet, int packetIdx)
{
	HalleyAssertDev(packet.seqIdx < 0x8000);
    HalleyAssertDev(packetIdx >= 0 && packetIdx < maxPacketQueueSize);

	// Only patch parts of the header we need to update.

    uint8_t* header = packet.data.data();
	size_t offs = sizeof(headerSignature2);

	header[offs++] = packet.parity;
    header[offs++] = packet.seqIdx >> 8;
    header[offs++] = packet.seqIdx & 0xff;
    header[offs++] = packet.subIdx;
	header[offs++] = packetIdx >> 8;
	header[offs++] = packetIdx & 0xff;
	header[offs++] = 1;
	header[offs++] = 0; // unused
	header[offs++] = 0; // unused

	HalleyAssertDebug(headerSize - offs == 8);

	packet.timestamp = Clock::now();

	// Always sets *some* bits, hence the 1 at header[11] - so we never use id=0.
	uint64_t datagramId;
	memcpy(&datagramId, &header[sizeof(headerSignature2)], 8);
	HalleyAssertDebug(datagramId != 0);

	doSendUnreliablePacket(packet.data.byte_span().subspan(0, headerSize + packet.dataSize), datagramId);
}

void AckUnreliableConnection::doSendUnreliablePacket(gsl::span<const std::byte> packet, uint64_t id) const
{
	parent->sendUnreliablePacket(packet, id);
}

void AckUnreliableConnection::beginSendUnreliablePackets()
{
	parent->beginSendUnreliablePackets();
}

void AckUnreliableConnection::flushSendUnreliablePackets()
{
	parent->flushSendUnreliablePackets();
}

bool AckUnreliableConnection::receive(InboundNetworkPacket& packet)
{
    auto status = parent->getStatus();
    if (status != ConnectionStatus::Connected) {
        return false;
    }

	// Need to send ack packets first, code below invalidates the sequence indices.
	doSendAckPackets();

    auto& slot = inbound.packets[inbound.curPacketIdx];

    if (slot.seqIdx < 0x8000) {
        if (slot.seqIdx != inbound.seqIndex.toSlot()) {
        	/*throw Exception("Unexpected packet sequence index:\n"
            	"found " + toString(slot.seqIdx) + ", parity " + toString((int) slot.parity) + "\n"
            	"expected " + toString(inbound.seqIndex.toSlot()) + ", parity " + toString((int) inbound.seqIndex.parity()),
            	HalleyExceptions::Network);*/
        	evictInboundQueue(inbound.seqIndex.toSlot(), inbound.seqIndex.parity());
        }

        if (slot.subIdx == 0) {
	      	packet = InboundNetworkPacket(slot.data.byte_span().subspan(headerSize, slot.dataSize));

        	slot.seqIdx = 0xffff;

        	networkStatsListener.onReceiveData(slot.dataSize, 1);

        	if (statsListener) {
        		statsListener->onPacketReceived(inbound.seqIndex.toSlot(), slot.dataSize, false);
        	}

        	inbound.curPacketIdx = (inbound.curPacketIdx + 1) % maxPacketQueueSize;
        	inbound.seqIndex.update();

        	return true;
        } else {
            // Check if all sub-packets have arrived.
            int numSubPackets = (slot.subIdx >> 4) + 1;
            HalleyAssertDev(numSubPackets > 1 && (slot.subIdx & 15) == 0);

            bool isComplete = true;
            for (size_t i = 1; isComplete && i < numSubPackets; i++) {
                const auto& sub = inbound.packets[(inbound.curPacketIdx + i) % maxPacketQueueSize];
            	if (sub.seqIdx == slot.seqIdx) {
            		HalleyAssertDev((sub.subIdx & 15) == i);
            		HalleyAssertDev((sub.subIdx >> 4) + 1 == numSubPackets);
            		HalleyAssertDev(sub.parity == slot.parity);
            	}
            	isComplete &= sub.seqIdx == slot.seqIdx && sub.parity == slot.parity;
                isComplete &= (sub.subIdx & 15) == i;
                isComplete &= (sub.subIdx >> 4) + 1 == numSubPackets;
            }

            if (isComplete) {
                size_t totalSize = 0;

                for (size_t i = 0; i < numSubPackets; i++) {
                    auto& sub = inbound.packets[(inbound.curPacketIdx + i) % maxPacketQueueSize];

                    memcpy(inboundCache.data() + totalSize, sub.data.data() + headerSize, sub.dataSize);
                    totalSize += sub.dataSize;

                    sub.seqIdx = 0xffff;
                }

                packet = InboundNetworkPacket(inboundCache.byte_span().subspan(0, totalSize));

                networkStatsListener.onReceiveData(totalSize, numSubPackets);

                if (statsListener) {
                    statsListener->onPacketReceived(inbound.seqIndex.toSlot(), totalSize, false);
                }

            	inbound.curPacketIdx = (inbound.curPacketIdx + numSubPackets) % maxPacketQueueSize;
                inbound.seqIndex.update();

                return true;
            }
        }
    }

	float resendDelay = parent->getUnreliablePacketResendTime(averagePacketAckTime);
	resendUnAckPackets(std::clamp(resendDelay, 0.05f, 2.0f));

    return false;
}

size_t AckUnreliableConnection::getMaxUnreliablePacketSize() const
{
	return totalMaxPacketSize - headerSize;
}

size_t AckUnreliableConnection::getRealMaxUnreliablePacketSize() const
{
	return realMaxPacketSize - headerSize;
}

void AckUnreliableConnection::onUnreliablePacketReceived(gsl::span<const std::byte> packet)
{
    size_t packetSize = packet.size();
    auto* header = reinterpret_cast<const uint8_t*>(packet.data());

    if (packetSize < 6 || memcmp(header, headerSignature2, 3) != 0) {
    	Logger::logWarning("rcv packet too small, or wrong signature");
        return;
    }

	uint8_t parity = header[3];
    uint16_t seqIdx = (static_cast<uint16_t>(header[4]) << 8) | header[5];

    if (seqIdx == 0xffff) {
    	// This is an ACK packet.
        onAckPacketsReceive(packet.subspan(6), parity);
        return;
    }

	if (!inbound.seqIndex.isInValidRange(SeqIndex::make(seqIdx, parity), 256)) {
		// The seqIdx/parity pair of this packet is far off the inbound queue.
		evictInboundQueue(seqIdx, parity);
	}

    uint8_t subIdx = header[6];
    int packetIdx = (header[7] << 8) | header[8];

	HalleyAssertDebug(header[9] == 1 && header[10] == 0 && header[11] == 0);

	size_t dataSize = packetSize - headerSize;

	bool isDupe = false;
	bool isExpired = false;

    auto& slot = inbound.packets[packetIdx];

	if (slot.seqIdx == seqIdx) {
		// Already got this packet.
		if (slot.subIdx != subIdx || slot.dataSize != dataSize) {
	        throw Exception("Rcv resent packet, but data mismatch", HalleyExceptions::Network);
		}
		if (statsListener) {
			statsListener->onPacketReceived(seqIdx, dataSize, true);
		}
		isDupe = true;
		slot.resend++;
	}

	isExpired = !isDupe && isExpiredSeqIndex(inbound, seqIdx, parity);

	if (!isDupe && !isExpired) {
		if (slot.seqIdx < 0x8000) {
			close("too many inbound packets");
		}

		slot.dataSize = dataSize;

		// Copy the full packet, including the header.
		memcpy(slot.data.data(), packet.data(), packetSize);

		slot.seqIdx = seqIdx;
		slot.subIdx = subIdx;
		slot.parity = parity;
		slot.resend = 0;
	}

	// Need to always ACK, even for duplicated and expired packets.

	int ackIdx = 0;
	while (ackIdx < numAckPackets) {
		// TODO: a hash set may be faster?
		if (ackPackets[ackIdx].first == packetIdx && ackPackets[ackIdx].second == seqIdx) {
			break;
		}
		ackIdx++;
	}

	if (ackIdx >= numAckPackets && numAckPackets < maxPacketQueueSize) {
		ackPackets[numAckPackets++] = {packetIdx, seqIdx};
	}
}

void AckUnreliableConnection::onUnreliablePacketAck(uint64_t id)
{
#if !SIMULATE_PLATFORM_ACK
	HalleyAssertDev(platformSupportsAck);
#endif

	uint8_t header[8];
	memcpy(header, &id, 8);

	uint8_t parity = header[0];
	uint16_t seqIdx = (static_cast<uint16_t>(header[1]) << 8) | header[2];
	uint8_t subIdx = header[3];
	int packetIdx = (header[4] << 8) | header[5];

	HalleyAssertDebug(header[6] == 1 && header[7] == 0 && header[8] == 0);

	auto& slot = outbound.packets[packetIdx];

	if (doProcessAckPacket(slot, packetIdx, seqIdx, parity)) {
		HalleyAssertDebug(subIdx == slot.subIdx);
		const Clock::time_point now = Clock::now();
		float latency = std::chrono::duration<float>(now - slot.timestamp).count();
		averagePacketAckTime = lerp(averagePacketAckTime, latency, 0.2f);
	}

	forwardOutboundQueue();
}

void AckUnreliableConnection::onUnreliablePacketLost(uint64_t id)
{
	HalleyAssertDev(platformSupportsAck);

	uint8_t header[8];
	memcpy(header, &id, 8);

	uint8_t parity = header[0];
	uint16_t seqIdx = (static_cast<uint16_t>(header[1]) << 8) | header[2];
	uint8_t subIdx = header[3];
	int packetIdx = (header[4] << 8) | header[5];

	HalleyAssertDebug(header[6] == 1 && header[7] == 0 && header[8] == 0);

	Logger::logDev(".. lost " + toString(packetIdx));

	auto& slot = outbound.packets[packetIdx];

	if (slot.seqIdx == seqIdx) {
		HalleyAssertDebug(subIdx == slot.subIdx);
		HalleyAssertDebug(parity == slot.parity);

		if (statsListener) {
			statsListener->onPackedLost(slot.seqIdx);
		}

		slot.lost = true;
	} else {
		Logger::logDev("rcv LOST for non-matching slot " + toString(packetIdx));
	}
}

void AckUnreliableConnection::doSendAckPackets()
{
	if (platformSupportsAck) {
		return;
	}

    if (numAckPackets == 0) {
        return;
    }

	// There's a limit how many ACKs fit into a packet.
	const int maxAcks = static_cast<int>(realMaxPacketSize - 6) / 4;
	const int numAcksToSend = std::min(maxAcks, numAckPackets);

	if (numAcksToSend != numAckPackets) {
		Logger::logWarning("Too many ACKs, sending " + toString(numAcksToSend) + "/" + toString(numAckPackets));
	}

    std::array<uint8_t, 6 + maxPacketQueueSize * 4> packet = {};

    uint8_t* msg = packet.data();

    msg[0] = headerSignature2[0];
    msg[1] = headerSignature2[1];
    msg[2] = headerSignature2[2];
    msg[3] = inbound.seqIndex.parity();

    msg[4] = 0xff;
    msg[5] = 0xff;

    for (int i = 0; i < numAcksToSend; i++) {
    	auto [packetIdx, seqIdx] = ackPackets[i];

        msg[6 + 4 * i] = packetIdx >> 8;
        msg[6 + 4 * i + 1] = packetIdx & 0xff;
        msg[6 + 4 * i + 2] = seqIdx >> 8;
        msg[6 + 4 * i + 3] = seqIdx & 0xff;
    }

    // NOTE: The ACKs themselves are sent unreliably.
    // The ACK mechanism on both sides makes sure that duplicates are caught.
	doSendUnreliablePacket(gsl::span<const std::byte>(reinterpret_cast<std::byte *>(packet.data()), 6 + 4 * numAcksToSend), 0);

	if (numAcksToSend != numAckPackets) {
		for (size_t n = 0; n < numAckPackets - numAcksToSend; n++) {
			ackPackets[n] = ackPackets[numAcksToSend + n];
		}
	}

	numAckPackets -= numAcksToSend;
}

void AckUnreliableConnection::onAckPacketsReceive(gsl::span<const std::byte> data, uint8_t parity)
{
	HalleyAssertDev(!platformSupportsAck);

    auto ackData = reinterpret_cast<const uint8_t *>(data.data());
    size_t size = data.size();

	HalleyAssertDev(size > 0 && (size % 4) == 0);

#if SIMULATE_PLATFORM_ACK
	for (size_t i = 0; i < size; i += 4) {
		int packetIdx = (ackData[i] << 8) | ackData[i + 1];
    	uint16_t seqIdx = (static_cast<uint16_t>(ackData[i + 2]) << 8) | ackData[i + 3];

		auto& slot = outbound.packets[packetIdx];

		uint8_t header[8];
		header[0] = parity;
		header[1] = seqIdx >> 8;
		header[2] = seqIdx & 0xff;
		header[3] = slot.subIdx;
		header[4] = packetIdx >> 8;
		header[5] = packetIdx & 0xff;
		header[6] = 1;
		header[7] = 0;

		uint64_t datagramId;
		memcpy(&datagramId, header, 8);

		onUnreliablePacketAck(datagramId);
	}
#else
	float avgLatencySum = 0.f;
	size_t avgLatencyCount = 0;
	const Clock::time_point now = Clock::now();

    for (size_t i = 0; i < size; i += 4) {
    	int packetIdx = (ackData[i] << 8) | ackData[i + 1];
    	uint16_t seqIdx = (static_cast<uint16_t>(ackData[i + 2]) << 8) | ackData[i + 3];

    	auto& slot = outbound.packets[packetIdx];

    	if (doProcessAckPacket(slot, packetIdx, seqIdx, parity)) {
    		avgLatencySum += std::chrono::duration<float>(now - slot.timestamp).count();
    		avgLatencyCount++;
    	}
    }

	// Now that this ACK packet has been fully processed, walk forward
	// the "first packet in use" index.
	forwardOutboundQueue();

	// Update average ACK time.
	if (avgLatencyCount > 0) {
		avgLatencySum /= static_cast<float>(avgLatencyCount);
		averagePacketAckTime = lerp(averagePacketAckTime, avgLatencySum, 0.2f);
	}
#endif
}

bool AckUnreliableConnection::doProcessAckPacket(SubPacket& slot, int packetIdx, uint16_t seqIdx, uint8_t parity)
{
	if (slot.seqIdx != 0xffff) {
		if (slot.seqIdx == seqIdx) {
			if (statsListener) {
				statsListener->onPacketAcked(slot.seqIdx);
			}

			slot.clear();

			return true;
		}

		if (!isExpiredSeqIndex(outbound, seqIdx, parity)) {
			// ACKs can be lost and resent too.
			Logger::logDev("rcv mismatch ACK for slot " + toString(packetIdx) + ", seqIdx " + toString(slot.seqIdx) + ", remote seqIdx " + toString(seqIdx));
		}
	} else if (!isExpiredSeqIndex(outbound, seqIdx, parity)) {
		// Resent packets can be ACKd more than once.
		Logger::logDev("rcv ACK for empty slot " + toString(packetIdx) + ", remote seqIdx " + toString(seqIdx));
	}

	return false;
}

void AckUnreliableConnection::forwardOutboundQueue()
{
	while (outbound.firstPacketIdx != outbound.curPacketIdx) {
		auto slot = &outbound.packets[outbound.firstPacketIdx];
		if (slot->seqIdx != 0xffff) {
			break;
		}
		outbound.firstPacketIdx = (outbound.firstPacketIdx + 1) % maxPacketQueueSize;
	}
}

void AckUnreliableConnection::resendUnAckPackets(float minResendTimeDiff)
{
	const Clock::time_point now = Clock::now();

	// This re-sends packets which are not yet acknowledged, or reported as "lost".
	// Stops at the first "gap", which would be a packet which we already got an ACK for.
	int idx = outbound.firstPacketIdx;
	do {
		auto slot = &outbound.packets[idx];

		if (slot->seqIdx == 0xffff) {
			break; // stop at the first index not in use
		}

		if (slot->dataSize == 0) {
			break;
		}

		if (!slot->lost) {
			float timeSinceSent = std::chrono::duration<float>(now - slot->timestamp).count();
			if (timeSinceSent < minResendTimeDiff) {
				break; // stop at first packet queued up too recently
			}
		}

		const auto* header = slot->data.data();

		uint64_t datagramId;
		memcpy(&datagramId, &header[sizeof(headerSignature2)], 8);

		doSendUnreliablePacket(slot->data.byte_span().subspan(0, headerSize + slot->dataSize), datagramId);

		if (statsListener) {
			statsListener->onPacketResent(slot->seqIdx);
		}

		slot->lost = false;
		slot->timestamp = now; // Update timestamp, or this will spam each frame.

		idx = (idx + 1) % maxPacketQueueSize;
	} while (idx != outbound.curPacketIdx);
}

float AckUnreliableConnection::getLatency() const
{
    return averagePacketAckTime;
}

void AckUnreliableConnection::setStatsListener(IAckUnreliableConnectionStatsListener* listener)
{
    statsListener = listener;
}

void AckUnreliableConnection::evictInboundQueue(uint16_t seqIdx, uint8_t parity)
{
	int lost = 0;

	for (auto& p: inbound.packets) {
		if (p.seqIdx != 0xffff) {
			lost++;
		}
		p.seqIdx = 0xffff;
		p.subIdx = 0;
		p.parity = parity;
	}

	if (lost > 0) {
		Logger::logError("Inbound network queue has been flushed, " + toString(lost) + " packets dropped.");
	}

	Logger::logWarning("Inbound network queue has been reset.");

	inbound.seqIndex = SeqIndex::make(seqIdx, parity);
}

bool AckUnreliableConnection::checkOutboundQueue(int numPacketsToSend) const
{
	int free = 0;

	while (free < numPacketsToSend) {
		int packetIdx = (outbound.curPacketIdx + free) % maxPacketQueueSize;
		auto& slot = outbound.packets[packetIdx];

		// If the slot is not in use, we are good. Step forward, and keep looking.
		if (slot.seqIdx == 0xffff) {
			free++;
			continue;
		}

		break;
	}

	return free >= numPacketsToSend;
}

bool AckUnreliableConnection::isExpiredSeqIndex(const InOutQueue& queue, uint16_t seqIdx, uint8_t parity)
{
	// Sequence indices wrap, so we can't just plain compare.
	const SeqIndex cmp = SeqIndex::make(seqIdx, parity);

	if (cmp.count < queue.seqIndex.count) {
		return true;
	}

	if (cmp.count >= queue.seqIndex.count + 0x8000) {
		return true;
	}

	return false;
}

bool AckUnreliableConnection::SeqIndex::isInValidRange(SeqIndex other, size_t range) const
{
	if (count < other.count) {
		return count + range >= other.count;
	} else {
		return other.count + range >= count;
	}
}

AckUnreliableConnection::SeqIndex AckUnreliableConnection::SeqIndex::make(uint16_t seqIdx, uint8_t parity)
{
	HalleyAssertDev(seqIdx < 0x8000);
	return { static_cast<size_t>(parity) << 15 | seqIdx };
}
