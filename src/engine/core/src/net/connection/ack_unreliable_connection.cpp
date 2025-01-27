#include "halley/net/connection/ack_unreliable_connection.h"
#include "halley/net/connection/network_packet.h"
#include "halley/net/connection/network_service.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/maths/random.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

using namespace Halley;

AckUnreliableConnection::AckUnreliableConnection(std::shared_ptr<IConnection> parent, INetworkServiceStatsListener& networkStatsListener)
    : parent(std::move(parent))
    , networkStatsListener(networkStatsListener)
{
    this->parent->setUnreliablePacketListener(this);

    maxPacketSize = this->parent->getMaxUnreliablePacketSize();

    inboundCache.resize_no_init(16 * maxPacketSize);

    for (auto &p: inbound.packets) {
        p.data.resize(maxPacketSize);
        p.seqIdx = 0xffff;
    }

    for (auto &p: outbound.packets) {
        p.data.resize(maxPacketSize);
        p.seqIdx = 0xffff;
    	memcpy(p.data.data(), headerSignature, 4);
    	memset(p.data.data() + 4, 0, maxPacketSize - 4);
    }
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
	Expects(type == TransmissionType::Unreliable);

	auto status = parent->getStatus();
	if (status != ConnectionStatus::Connected) {
		return;
	}

	if (tryCacheSmallPacket(packet)) {
		return;
	}

	doFlushSmallPackets();

	doSend(packet.getBytes(), false);
}

void AckUnreliableConnection::doSend(gsl::span<const gsl::byte> packet, bool small)
{
    size_t size = packet.size();
    size_t maxSize = maxPacketSize - headerSize;

    if (maxSize >= size) {
        auto& slot = outbound.packets[outbound.curPacketIdx];

        if (slot.seqIdx < 0x8000) {
            close("outbound packet queue is full");
            return;
        }

    	if (small) {
    		// This is a small packet cache - need to copy parts of the cached header.
    		const auto& cache = outbound.packets[256];
    		memcpy(slot.data.data() + 8, cache.data.data() + 8, cache.subIdx);
    	}

	    networkStatsListener.onSendData(size, 1);

    	slot.dataSize = size;
        memcpy(slot.data.data() + headerSize, packet.data(), size);
        slot.seqIdx = outbound.curSeqIdx;
        slot.subIdx = 0;

        doSend(slot, outbound.curPacketIdx);

        outbound.curPacketIdx = (outbound.curPacketIdx + 1) % 256;
    } else {
    	Ensures(!small);

        size_t numSubPackets = size / maxSize;
        if (size % maxSize != 0) {
            numSubPackets++;
        }

        if (numSubPackets > 16) {
            throw Exception("Packet size too large: " + toString(size) + " bytes", HalleyExceptions::Network);
        }

        for (size_t i = 0; i < numSubPackets; i++) {
            auto& slot = outbound.packets[outbound.curPacketIdx];

            if (slot.seqIdx < 0x8000) {
	            close("outbound packet queue is full");
                return;
            }

            slot.dataSize = std::min(packet.size(), maxSize);
            memcpy(slot.data.data() + headerSize, packet.data(), slot.dataSize);

            packet = packet.subspan(slot.dataSize);

            slot.seqIdx = outbound.curSeqIdx;
            slot.subIdx = uint8_t(i) | uint8_t(numSubPackets << 4);

            doSend(slot, outbound.curPacketIdx);

            outbound.curPacketIdx = (outbound.curPacketIdx + 1) % 256;
        }

    	networkStatsListener.onSendData(size, numSubPackets);

        Expects(packet.empty());
    }

    if (statsListener) {
        statsListener->onPacketSent(outbound.curSeqIdx, size);
    }

    outbound.curSeqIdx = (outbound.curSeqIdx + 1) % 0x8000;
}

bool AckUnreliableConnection::tryCacheSmallPacket(const OutboundNetworkPacket& packet)
{
	const size_t size = packet.getSize();

	if (size >= 256) {
		return false;
	}

	// Uses outbound queue index 256 to cache small packets.

	auto& slot = outbound.packets[256];

	if (slot.subIdx > 7) {
		return false; // too many small packets in cache already
	}

	size_t maxSize = maxPacketSize - headerSize;
	if (maxSize < slot.dataSize + size) {
		return false; // would overflow buffer size
	}

	// copy the packet data
	slot.dataSize += packet.copyTo(slot.data.byte_span().subspan(headerSize + slot.dataSize));

	// update header and counter
	slot.data.data()[8 + slot.subIdx] = (uint8_t) size;
	slot.subIdx++;

	return true;
}

void AckUnreliableConnection::doSend(SubPacket& packet, int packetIdx)
{
    Expects(packetIdx >= 0 && packetIdx < 256);

	// Only patch parts of the header we need to update.

    uint8_t* header = packet.data.data();

    header[4] = packet.seqIdx >> 8;
    header[5] = packet.seqIdx & 0xff;
    header[6] = packet.subIdx;
    header[7] = packetIdx & 0xff;

	packet.timestamp = Clock::now();

	doSendUnreliablePacket(packet.data.byte_span().subspan(0, headerSize + packet.dataSize), packet.seqIdx);
}

void AckUnreliableConnection::doSendUnreliablePacket(gsl::span<const gsl::byte> packet, uint16_t seqIdx)
{
#ifdef DEV_BUILD
	if (simulatePacketLoss > 0.0f) {
		if (Random::getGlobal().getFloat(0.0f, 1.0f) < simulatePacketLoss) {
			return;
		}
	}
#endif

	parent->sendUnreliablePacket(packet);
}

void AckUnreliableConnection::flushOutboundQueue()
{
	doFlushSmallPackets();
}

void AckUnreliableConnection::doFlushSmallPackets()
{
	auto& slot = outbound.packets[256];

	if (slot.subIdx > 0) {
		doSend(slot.data.const_byte_span().subspan(headerSize, slot.dataSize), true);
		memset(slot.data.data() + 8, 0, 8);

		slot.dataSize = 0;
		slot.subIdx = 0;
	}
}

bool AckUnreliableConnection::receive(InboundNetworkPacket& packet)
{
    auto status = parent->getStatus();
    if (status != ConnectionStatus::Connected) {
        return false;
    }

	// Try cached small packets first.
	if (tryReceiveSmallPacket(packet)) {
		return true;
	}

	// Need to send ack packets first, code below invalidates the sequence indices.
	doSendAckPackets();

    auto& slot = inbound.packets[inbound.curPacketIdx];

    if (slot.seqIdx < 0x8000) {
        if (slot.seqIdx != inbound.curSeqIdx) {
            throw Exception("Unexpected packet sequence index", HalleyExceptions::Network);
        }

        if (slot.subIdx == 0) {
	        if (slot.data[8] != 0) {
        		// This is a "small packets" packet. Copy to sub-packet slot #256.
        		auto& cache = inbound.packets[256];
        		memcpy(cache.data.data(), slot.data.data(), headerSize + slot.dataSize);

        		cache.subIdx = 1; // used as counter
        		size_t offset = slot.data[8];
        		for (int i = 1; i < 8; i++, cache.subIdx++) {
        			const size_t sz = cache.data[8 + i];
        			if (sz == 0) {
        				break;
        			}
        			offset += sz;
        		}

        		Ensures(offset == slot.dataSize);
        		cache.dataSize = headerSize; // used as offset

        		// Return the first small packet
        		tryReceiveSmallPacket(packet);
        	} else {
	      		packet = InboundNetworkPacket(slot.data.byte_span().subspan(headerSize, slot.dataSize));
        	}

        	slot.seqIdx = 0xffff;

        	networkStatsListener.onReceiveData(slot.dataSize, 1);

        	if (statsListener) {
        		statsListener->onPacketReceived(inbound.curSeqIdx, slot.dataSize, false);
        	}

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

                    memcpy(inboundCache.data() + totalSize, sub.data.data() + headerSize, sub.dataSize);
                    totalSize += sub.dataSize;

                    sub.seqIdx = 0xffff;
                }

                packet = InboundNetworkPacket(inboundCache.byte_span().subspan(0, totalSize));

                networkStatsListener.onReceiveData(totalSize, numSubPackets);

                if (statsListener) {
                    statsListener->onPacketReceived(inbound.curSeqIdx, totalSize, false);
                }

            	inbound.curPacketIdx = (inbound.curPacketIdx + numSubPackets) % 256;
                inbound.curSeqIdx = (inbound.curSeqIdx + 1) % 0x8000;

                return true;
            }
        }
    }

	resendUnAckPackets(std::clamp(lag * 1.5f, 0.025f, 1.0f));

    return false;
}

bool AckUnreliableConnection::tryReceiveSmallPacket(InboundNetworkPacket& packet)
{
	auto& slot = inbound.packets[256];

	if (slot.subIdx == 0) {
		return false;
	}

	for (uint8_t i = 0; i < slot.subIdx; i++) {
		const uint8_t size = slot.data[8 + i];

		if (size == 0) {
			continue;
		}

		packet = InboundNetworkPacket(slot.data.byte_span().subspan(slot.dataSize, size));

		slot.data[8 + i] = 0;
		slot.dataSize += size;

		return true;
	}

	// all done, "flush"
	slot.subIdx = 0;

	return false;
}

void AckUnreliableConnection::onSend(gsl::span<const gsl::byte> packet)
{
}

void AckUnreliableConnection::onReceive(gsl::span<const gsl::byte> packet)
{
    size_t packetSize = packet.size();
    auto* header = (const uint8_t*) packet.data();

    if (packetSize < 6 || memcmp(header, headerSignature, 4) != 0) {
    	Logger::logWarning("rcv packet too small, or wrong signature");
        return;
    }

    uint16_t seqIdx = uint16_t(header[4] << 8) | header[5];

    if (seqIdx == 0xffff) {
    	// This is an ACK packet.
        onAckPacketsReceive(packet.subspan(6));
        return;
    }

    uint8_t subIdx = header[6];
    int packetIdx = header[7];

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
	}

	isExpired = !isDupe && isExpiredSeqIndex(inbound, seqIdx);

	if (!isDupe && !isExpired) {
		if (slot.seqIdx < 0x8000) {
			close("too many inbound packets");
		}

		slot.dataSize = dataSize;

		// Copy the full packet, including the header.
		memcpy(slot.data.data(), packet.data(), packetSize);

		slot.seqIdx = seqIdx;
		slot.subIdx = subIdx;
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

	if (ackIdx >= numAckPackets) {
		Expects(numAckPackets < 256);
		ackPackets[numAckPackets++] = {packetIdx, seqIdx};
	}
}

void AckUnreliableConnection::doSendAckPackets()
{
    if (numAckPackets == 0) {
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

    for (int i = 0; i < numAckPackets; i++) {
    	auto [slotIdx, seqIdx] = ackPackets[i];

        msg[6 + 3 * i] = slotIdx;
        msg[6 + 3 * i + 1] = seqIdx >> 8;
        msg[6 + 3 * i + 2] = seqIdx & 0xff;
    }

    // NOTE: This isn't sent reliably either. The ACK mechanism on both sides
	// makes sure that duplicates are caught.
	doSendUnreliablePacket(gsl::span<const gsl::byte>(reinterpret_cast<gsl::byte *>(packet.data()), 6 + 3 * numAckPackets), 0xffff);

	numAckPackets = 0;
}

void AckUnreliableConnection::onAckPacketsReceive(gsl::span<const gsl::byte> data)
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

    			slot->seqIdx = 0xffff;

    			// Clear header bytes
    			memset(slot->data.data() + 4, 0, headerSize - 4);

    			avgLatencySum += std::chrono::duration<float>(now - slot->timestamp).count();
    			avgLatencyCount++;
    		} else if (!isExpiredSeqIndex(outbound, seqIdx)) {
    			// ACKs can be lost and resent too.
	    		Logger::logDev("rcv mismatch ACK for slot " + toString((int) packetIdx) + ", seqIdx " + toString(slot->seqIdx) + ", remote seqIdx " + toString(seqIdx));
    		}
    	} else if (!isExpiredSeqIndex(outbound, seqIdx)) {
    		// Resent packets can be ACKd more than once.
   			Logger::logDev("rcv ACK for empty slot " + toString((int) packetIdx) + ", remote seqIdx " + toString(seqIdx));
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

	// Update latency
	if (avgLatencyCount > 0) {
		avgLatencySum /= static_cast<float>(avgLatencyCount);
		lag = lerp(lag, avgLatencySum, 0.2f);
	}
}

void AckUnreliableConnection::resendUnAckPackets(float minResendTimeDiff)
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

float AckUnreliableConnection::getLatency() const
{
    return lag;
}

void AckUnreliableConnection::setStatsListener(IAckUnreliableConnectionStatsListener* listener)
{
    statsListener = listener;
}

bool AckUnreliableConnection::isExpiredSeqIndex(const InOutQueue& queue, uint16_t seqIdx)
{
	// Sequence indices wrap, so we can't just plain compare.
	if (seqIdx > queue.curSeqIdx) {
		return (static_cast<size_t>(queue.curSeqIdx) + 4096) < seqIdx;
	}

	return seqIdx < queue.curSeqIdx;
}
