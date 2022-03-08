#include "connection/ack_unreliable_connection_stats.h"

using namespace Halley;

void AckUnreliableConnectionStats::update(Time time)
{
}

void AckUnreliableConnectionStats::onPacketSent(uint16_t sequence)
{
}

void AckUnreliableConnectionStats::onPacketResent(uint16_t sequence)
{
}

void AckUnreliableConnectionStats::onPacketAcked(uint16_t sequence)
{
}

gsl::span<const AckUnreliableConnectionStats::PacketStats> AckUnreliableConnectionStats::getPacketStats() const
{
	return packetStats;
}
