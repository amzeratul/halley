#include "reliable_connection.h"
#include <network_packet.h>
#include <iostream>

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
	, waitingAcks(BUFFER_SIZE)
	, tags(BUFFER_SIZE)
{
}

void ReliableConnection::close()
{
	parent->close();
}

ConnectionStatus ReliableConnection::getStatus() const
{
	return parent->getStatus();
}

void ReliableConnection::send(NetworkPacket&& packet)
{
	internalSend(packet, -1);
}

void ReliableConnection::sendTagged(NetworkPacket&& packet, int tag)
{
	Expects(tag >= 0);
	internalSend(packet, tag);
}

bool ReliableConnection::receive(NetworkPacket& packet)
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

void ReliableConnection::internalSend(NetworkPacket& packet, int tag)
{
	ReliableHeader header;
	header.sequence = sequenceSent++;
	header.ack = highestReceived;
	header.ackBits = generateAckBits();
	packet.addHeader(gsl::span<ReliableHeader>(&header, sizeof(ReliableHeader)));
	parent->send(std::move(packet));

	// Mark as waiting
	std::cout << "Sent " << header.sequence << "\n";
	size_t idx = header.sequence % BUFFER_SIZE;
	waitingAcks[idx] = true;
	tags[idx] = tag;
}

bool ReliableConnection::processReceivedPacket(NetworkPacket& packet)
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
	size_t idx = sequence % BUFFER_SIZE;
	if (waitingAcks[idx]) {
		waitingAcks[idx] = false;
		if (tags[idx] != -1) {
			for (auto& listener : ackListeners) {
				listener->onPacketAcked(tags[idx]);
			}
		}
		std::cout << "Ack " << sequence << "\n";
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
