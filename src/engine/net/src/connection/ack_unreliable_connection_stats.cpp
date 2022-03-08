#include "connection/ack_unreliable_connection_stats.h"

using namespace Halley;

AckUnreliableConnectionStats::AckUnreliableConnectionStats(size_t capacity, size_t lineSize)
	: capacity(capacity)
	, lineSize(lineSize)
{
	packetStats.resize(capacity);
}

void AckUnreliableConnectionStats::update(Time time)
{
}

void AckUnreliableConnectionStats::onPacketSent(uint16_t sequence, size_t size)
{
	packetStats[pos] = PacketStats{ sequence, State::Sent, size };
	pos = (pos + 1) % packetStats.size();

	// Upon reaching a new line, clear it
	if (pos % lineSize == 0) {
		lineStart = pos;
		for (size_t i = 0; i < lineSize; ++i) {
			packetStats[(pos + i) % packetStats.size()] = PacketStats();
		}
	}
}

void AckUnreliableConnectionStats::onPacketResent(uint16_t sequence)
{
	for (auto& packet: packetStats) {
		if (packet.seq == sequence) {
			packet.state = State::Resent;
			return;
		}
	}
}

void AckUnreliableConnectionStats::onPacketAcked(uint16_t sequence)
{
	for (auto& packet: packetStats) {
		if (packet.seq == sequence) {
			packet.state = State::Acked;
			return;
		}
	}
}

gsl::span<const AckUnreliableConnectionStats::PacketStats> AckUnreliableConnectionStats::getPacketStats() const
{
	return packetStats;
}

size_t AckUnreliableConnectionStats::getLineStart() const
{
	return lineStart;
}

size_t AckUnreliableConnectionStats::getLineSize() const
{
	return lineSize;
}
