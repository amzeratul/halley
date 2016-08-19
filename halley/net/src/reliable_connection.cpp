#include "reliable_connection.h"
#include <network_packet.h>
#include <iostream>
#include <chrono>
#include <halley/utils/utils.h>

using namespace Halley;

struct ReliableHeader
{
	unsigned short sequence = 0xFFFF;
	unsigned short ack = 0xFFFF;
	unsigned int ackBits = 0xFFFFFFFF;
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

void ReliableConnection::send(OutboundNetworkPacket&& packet)
{
	internalSend(packet, -1);
}

void ReliableConnection::sendTagged(OutboundNetworkPacket&& packet, int tag)
{
	Expects(tag >= 0);
	internalSend(packet, tag);
}

bool ReliableConnection::receive(InboundNetworkPacket& packet)
{
	// Keep trying until either:
	// a. upstream connection is out of packets (returns false)
	// b. processReceivedPacket returns true (returns true)

	do {
		bool result = parent->receive(packet);
		if (!result) {
			return false;
		}
	} while (!processReceivedPacket(packet));

	lastReceive = Clock::now();
	return true;
}

void ReliableConnection::addAckListener(IReliableConnectionAckListener& listener)
{
	ackListeners.push_back(&listener);
}

void ReliableConnection::removeAckListener(IReliableConnectionAckListener& listener)
{
	ackListeners.erase(std::find(ackListeners.begin(), ackListeners.end(), &listener));
}

void ReliableConnection::internalSend(OutboundNetworkPacket& packet, int tag)
{
	ReliableHeader header;
	header.sequence = sequenceSent++;
	header.ack = highestReceived;
	header.ackBits = generateAckBits();
	packet.addHeader(gsl::span<ReliableHeader>(&header, sizeof(ReliableHeader)));
	parent->send(std::move(packet));

	// Store send information
	std::cout << "Sent " << header.sequence << "\n";
	size_t idx = header.sequence % BUFFER_SIZE;
	auto& sent = sentPackets[idx];
	sent.waiting = true;
	sent.tag = tag;
	lastSend = sent.timestamp = Clock::now();
}

bool ReliableConnection::processReceivedPacket(InboundNetworkPacket& packet)
{
	ReliableHeader header;
	packet.extractHeader(gsl::span<ReliableHeader>(&header, sizeof(ReliableHeader)));
	unsigned short seq = header.sequence;

	size_t bufferPos = size_t(seq) % BUFFER_SIZE;
	unsigned short diff = seq - highestReceived;

	if (diff != 0 && diff < 0x8000) { // seq higher than highestReceived, with unsigned wrap-around
		if (diff > BUFFER_SIZE - 32) {
			// Ops, skipped too many packets!
			close();
			return false;
		}

		// Clear everything inbetween these. This is wrap-around safe.
		size_t expectedNextPos = size_t(highestReceived + 1) % BUFFER_SIZE;
		for (size_t i = expectedNextPos; i != bufferPos; i = (i + 1) % BUFFER_SIZE) {
			receivedSeqs[i] = false;
		}

		highestReceived = seq;
	}

	if (receivedSeqs[bufferPos]) {
		// Already received
		std::cout << "Rejected\n";
		return false;
	}

	// Mark this packet as received
	receivedSeqs[bufferPos] = true;

	// Process ack
	processReceivedAcks(header.ack, header.ackBits);

	return true;
}

void ReliableConnection::processReceivedAcks(unsigned short ack, unsigned int ackBits)
{
	// If acking something too far back in the past, ignore it
	unsigned short diff = sequenceSent - ack;
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
		std::cout << "Ack " << sequence << " with " << msgLag << " ms lag (" << lag << " overall lag).\n";
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

