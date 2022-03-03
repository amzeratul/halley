#include "connection/ack_unreliable_connection.h"
#include "connection/network_packet.h"
#include <iostream>
#include <chrono>
#include <utility>
#include <halley/utils/utils.h>
#include <halley/support/exception.h>

#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"
#include "halley/text/encode.h"
#include "halley/text/string_converter.h"

using namespace Halley;

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

void AckUnreliableConnection::sendTagged(gsl::span<AckUnreliableSubPacket> subPackets)
{
	std::array<gsl::byte, 2048> buffer;
	const auto dst = gsl::span<gsl::byte>(buffer);

	auto s = Serializer(dst, SerializerOptions(SerializerOptions::maxVersion));

	// Add header
	AckUnreliableHeader header;
	header.sequence = nextSequenceToSend;
	header.ack = highestReceived;
	header.ackBits = generateAckBits();
	s << header;

	// Add subpackets
	for (auto& subPacket : subPackets) {
		const uint16_t sizeAndResend = static_cast<uint16_t>(subPacket.data.size() << 1) | static_cast<uint16_t>(subPacket.resends ? 1 : 0);
		s << sizeAndResend;
		if (subPacket.resends) {
			s << subPacket.resendSeq;
		}
		s << gsl::span<const gsl::byte>(subPacket.data);

		// Get sequence
		const uint16_t seq = nextSequenceToSend++;
		const size_t idx = seq % BUFFER_SIZE;
		auto& sent = sentPackets[idx];
		sent.waiting = true;
		sent.tag = subPacket.tag;
		lastSend = sent.timestamp = Clock::now();

		// Update caller on the sequence number of this
		subPacket.seq = seq;
		if (subPacket.resends) {
			//std::cout << "Re-sending " << subPacket.resendSeq << " as " << seq << std::endl;
		}
	}

	// Send
	parent->send(TransmissionType::Unreliable, OutboundNetworkPacket(dst.subspan(0, s.getSize())));
}

void AckUnreliableConnection::processReceivedPacket(InboundNetworkPacket& packet)
{
	//Logger::logDev("Received packet with " + toString(packet.getSize()) + " bytes: " + Encode::encodeBase16(packet.getBytes()));
	auto s = Deserializer(packet.getBytes(), SerializerOptions(SerializerOptions::maxVersion));

	AckUnreliableHeader header;
	s >> header;

	processReceivedAcks(header.ack, header.ackBits);
	uint16_t seq = header.sequence;

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
		std::array<char, 2048> buffer;
		if (size > buffer.size() || size > s.getBytesLeft()) {
			throw Exception("Unexpected sub-packet size: " + toString(size) + " bytes, " + toString(s.getBytesLeft()) + " bytes remaining.", HalleyExceptions::Network);
		}
		auto subPacketData = gsl::as_writable_bytes(gsl::span<char>(buffer).subspan(0, size));
		s >> subPacketData;

		// Process sub-packet
		if (onSeqReceived(seq, resend, resendOf)) {
			pendingPackets.emplace_back(subPacketData);
		}
		++seq;
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

	for (int i = 32; --i >= 0; ) {
		if (ackBits & (1 << i)) {
			uint16_t seq = static_cast<uint16_t>(ack - (i + 1));
			onAckReceived(seq);
		}
	}
	onAckReceived(ack);
}

bool AckUnreliableConnection::onSeqReceived(uint16_t seq, bool isResend, uint16_t resendOf)
{
	size_t bufferPos = size_t(seq) % BUFFER_SIZE;
	size_t resendPos = size_t(resendOf) % BUFFER_SIZE;
	uint16_t diff = seq - highestReceived;

	if (diff != 0 && diff < 0x8000) { // seq higher than highestReceived, with unsigned wrap-around
		if (diff > BUFFER_SIZE - 32) {
			// Ops, skipped too many packets!
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

	if (receivedSeqs[bufferPos] != 0 || (isResend && receivedSeqs[resendPos] != 0)) {
		// Already received
		return false;
	}

	// Mark this packet as received
	receivedSeqs[bufferPos] |= 1;
	if (isResend) {
		receivedSeqs[resendPos] |= 2;
	}

	return true;
}

void AckUnreliableConnection::onAckReceived(uint16_t sequence)
{
	auto& data = sentPackets[sequence % BUFFER_SIZE];
	if (data.waiting) {
		data.waiting = false;
		if (data.tag != -1) {
			for (const auto& listener: ackListeners) {
				listener->onPacketAcked(data.tag);
			}
		}
		const float msgLag = std::chrono::duration<float>(Clock::now() - data.timestamp).count();
		reportLatency(msgLag);
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

void AckUnreliableConnection::reportLatency(float lastMeasuredLag)
{
	if (fabs(lag) < 0.00001f) {
		lag = lastMeasuredLag;
	} else {
		lag = lerp(lag, lastMeasuredLag, 0.2f);
	}
}

float AckUnreliableConnection::getTimeSinceLastSend() const
{
	return std::chrono::duration<float>(Clock::now() - lastSend).count();
}

float AckUnreliableConnection::getTimeSinceLastReceive() const
{
	return std::chrono::duration<float>(Clock::now() - lastReceive).count();
}

