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
        p.data.resize_no_init(maxPacketSize);
        p.seqIdx = 0xffff;
    }

    for (auto &p: outbound.packets) {
        p.data.resize_no_init(maxPacketSize);
        p.seqIdx = 0xffff;
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

void AckUnreliableConnection::send(TransmissionType type, OutboundNetworkPacket packet)
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
            close("outbound packet queue is full");
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
	            close("outbound packet queue is full");
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

void AckUnreliableConnection::doSend(SubPacket& packet, int packetIdx)
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

            	inbound.curPacketIdx = (inbound.curPacketIdx + numSubPackets) % 256;
                inbound.curSeqIdx = (inbound.curSeqIdx + 1) % 0x8000;

                return true;
            }
        }
    }

	resendUnAckPackets(std::clamp(lag * 1.5f, 0.025f, 1.0f));

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

    size_t connIdLen = 0;
    /*size_t connIdLen = 1;
    if ((header[15] & 0x80) != 0) {
        connIdLen = 2;
    }*/

	size_t dataSize = packetSize - headerSize + connIdLen;

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
		memcpy(slot.data.data(), packet.data() + headerSize - connIdLen, dataSize);

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
