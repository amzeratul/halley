#include "connection/reliable_connection.h"
#include "connection/network_packet.h"
#include <iostream>
#include <chrono>
#include <halley/utils/utils.h>
#include <halley/support/exception.h>
#include "halley/text/string_converter.h"

using namespace Halley;

struct ReliableHeader
{
	uint16_t sequence = 0xFFFF;
	uint16_t ack = 0xFFFF;
	uint32_t ackBits = 0xFFFFFFFF;
};

struct ReliableSubHeader
{
	uint8_t sizeA;
	uint8_t sizeB;
	uint16_t resend;
};

constexpr size_t BUFFER_SIZE = 1024;

ReliableConnection::ReliableConnection(std::shared_ptr<IConnection> parent)
	: parent(parent)
	, receivedSeqs(BUFFER_SIZE)
	, sentPackets(BUFFER_SIZE)
{
	lastSend = lastReceive = Clock::now();
}

void ReliableConnection::close()
{
	parent->close();
}

ConnectionStatus ReliableConnection::getStatus() const
{
	return parent->getStatus();
}

bool ReliableConnection::isSupported(TransmissionType type) const
{
	return true;
}

void ReliableConnection::send(TransmissionType type, OutboundNetworkPacket packet)
{
	ReliableSubPacket subPacket;
	subPacket.data.resize(packet.getSize());
	packet.copyTo(subPacket.data);
	subPacket.resends = false;
	subPacket.tag = -1;

	sendTagged(gsl::span<ReliableSubPacket>(&subPacket, 1));
}

void ReliableConnection::sendTagged(gsl::span<ReliableSubPacket> subPackets)
{
	unsigned short firstSeq = nextSequenceToSend;
	std::array<gsl::byte, 2048> buffer;
	gsl::span<gsl::byte, 2048> dst(buffer);
	size_t pos = sizeof(ReliableHeader);

	for (auto& subPacket : subPackets) {
		// Add reliable sub-header
		bool isResend = subPacket.resends;
		unsigned short resending = subPacket.resendSeq;
		size_t size = subPacket.data.size();
		bool longSize = size >= 64;
		if (longSize) {
			std::array<unsigned char, 2> b;
			b[0] = static_cast<unsigned char>((size >> 8) & 0x3F) | 0x40 | (isResend ? 0x80 : 0);
			b[1] = static_cast<unsigned char>(size & 0xFF);
			memcpy(dst.subspan(pos, 2).data(), b.data(), 2);
			pos += 2;
		} else {
			unsigned char b = static_cast<unsigned char>(size) | (isResend ? 0x80 : 0);
			memcpy(dst.subspan(pos, 1).data(), &b, 1);
			pos += 1;
		}
		if (resending) {
			memcpy(dst.subspan(pos, 2).data(), &resending, 2);
			pos += 2;
		}

		// Add data
#ifdef _MSC_VER
		memcpy_s(dst.subspan(pos).data(), dst.subspan(pos).size_bytes(), subPacket.data.data(), subPacket.data.size());
#else
		memcpy(dst.subspan(pos).data(), subPacket.data.data(), subPacket.data.size());
#endif
		pos += subPacket.data.size();

		// Get sequence
		unsigned short seq = nextSequenceToSend++;
		size_t idx = seq % BUFFER_SIZE;
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

	// Add reliable header
	ReliableHeader header;
	header.sequence = firstSeq;
	header.ack = highestReceived;
	header.ackBits = generateAckBits();
	auto headerData = gsl::as_bytes(gsl::span<ReliableHeader>(&header, 1));
#ifdef _MSC_VER
	memcpy_s(dst.data(), dst.size_bytes(), headerData.data(), headerData.size());
#else
	memcpy(dst.data(), headerData.data(), headerData.size());
#endif

	// Send
	parent->send(TransmissionType::Unreliable, OutboundNetworkPacket(dst.subspan(0, pos)));
}

bool ReliableConnection::receive(InboundNetworkPacket& packet)
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

void ReliableConnection::addAckListener(IReliableConnectionAckListener& listener)
{
	ackListeners.push_back(&listener);
}

void ReliableConnection::removeAckListener(IReliableConnectionAckListener& listener)
{
	ackListeners.erase(std::find(ackListeners.begin(), ackListeners.end(), &listener));
}

void ReliableConnection::processReceivedPacket(InboundNetworkPacket& packet)
{
	ReliableHeader header;
	packet.extractHeader(gsl::as_writable_bytes(gsl::span<ReliableHeader>(&header, 1)));
	processReceivedAcks(header.ack, header.ackBits);
	unsigned short seq = header.sequence;

	while (packet.getSize() > 0) {
		if (packet.getSize() < 1) {
			throw Exception("Sub-packet header not found.", HalleyExceptions::Network);
		}

		// Sub-packets header
		std::array<unsigned char, 2> sizeBytes;
		auto data = gsl::as_writable_bytes(gsl::span<unsigned char>(sizeBytes));
		packet.extractHeader(data.subspan(0, 1));
		size_t size = 0;
		bool resend = (sizeBytes[0] & 0x80) != 0;
		if (sizeBytes[0] & 0x40) {
			if (packet.getSize() < 1) {
				throw Exception("Sub-packet header incomplete.", HalleyExceptions::Network);
			}
			packet.extractHeader(data.subspan(1, 1));
			size = static_cast<unsigned short>(sizeBytes[0] & 0x3F) << 8 | sizeBytes[1];
		} else {
			size = sizeBytes[0] & 0x3F;
		}
		unsigned short resendOf = 0;
		if (resend) {
			if (packet.getSize() < 2) {
				throw Exception("Sub-packet header missing resend data", HalleyExceptions::Network);
			}
			packet.extractHeader(gsl::as_writable_bytes(gsl::span<unsigned short>(&resendOf, 1)));
		}

		// Extract data
		std::array<char, 2048> buffer;
		if (size > buffer.size() || size > packet.getSize()) {
			throw Exception("Unexpected sub-packet size: " + toString(size) + " bytes, packet is " + toString(packet.getSize()) + " bytes.", HalleyExceptions::Network);
		}
		auto subPacketData = gsl::span<char, 2048>(buffer).subspan(0, size);
		packet.extractHeader(gsl::as_writable_bytes(subPacketData));

		// Process sub-packet
		if (onSeqReceived(seq, resend, resendOf)) {
			pendingPackets.push_back(InboundNetworkPacket(gsl::as_bytes(subPacketData)));
		}
		++seq;
	}
}

void ReliableConnection::processReceivedAcks(unsigned short ack, unsigned int ackBits)
{
	// If acking something too far back in the past, ignore it
	unsigned short diff = nextSequenceToSend - ack;
	if (diff > 512) {
		return;
	}

	for (int i = 32; --i >= 0; ) {
		if (ackBits & (1 << i)) {
			unsigned short seq = static_cast<unsigned short>(ack - (i + 1));
			onAckReceived(seq);
		}
	}
	onAckReceived(ack);
}

bool ReliableConnection::onSeqReceived(unsigned short seq, bool isResend, unsigned short resendOf)
{
	size_t bufferPos = size_t(seq) % BUFFER_SIZE;
	size_t resendPos = size_t(resendOf) % BUFFER_SIZE;
	unsigned short diff = seq - highestReceived;

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

void ReliableConnection::onAckReceived(unsigned short sequence)
{
	auto& data = sentPackets[sequence % BUFFER_SIZE];
	if (data.waiting) {
		data.waiting = false;
		if (data.tag != -1) {
			for (auto& listener : ackListeners) {
				listener->onPacketAcked(data.tag);
			}
		}
		float msgLag = std::chrono::duration<float>(Clock::now() - data.timestamp).count();
		reportLatency(msgLag);
	}
}

unsigned int ReliableConnection::generateAckBits()
{
	unsigned int result = 0;
	
	for (size_t i = 0; i < 32; i++) {
		size_t bufferPos = ((highestReceived - 1 - i) + 0x10000) % BUFFER_SIZE;
		result |= static_cast<unsigned int>(1 & receivedSeqs[bufferPos]) << i;
	}

	return result;
}

void ReliableConnection::reportLatency(float lastMeasuredLag)
{
	if (fabs(lag) < 0.00001f) {
		lag = lastMeasuredLag;
	} else {
		lag = lerp(lag, lastMeasuredLag, 0.2f);
	}
}

float ReliableConnection::getTimeSinceLastSend() const
{
	return std::chrono::duration<float>(Clock::now() - lastSend).count();
}

float ReliableConnection::getTimeSinceLastReceive() const
{
	return std::chrono::duration<float>(Clock::now() - lastReceive).count();
}

