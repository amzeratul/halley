#include "connection/instability_simulator.h"
#include <halley/maths/random.h>
#include <chrono>

using namespace Halley;

InstabilitySimulator::DelayedPacket::DelayedPacket(std::chrono::system_clock::time_point when, OutboundNetworkPacket packet)
	: when(when)
	, packet(packet)
{}

bool InstabilitySimulator::DelayedPacket::operator<(const DelayedPacket& other) const
{
	return when > other.when;
}

bool InstabilitySimulator::DelayedPacket::isReady() const
{
	return std::chrono::system_clock::now() >= when;
}

InstabilitySimulator::InstabilitySimulator(std::shared_ptr<IConnection> parent, float avgLag, float lagVariance, float packetLoss, float duplication)
	: parent(parent)
	, avgLag(avgLag)
	, lagVariance(lagVariance)
	, packetLoss(packetLoss)
	, duplication(duplication)
{
	Expects(packetLoss < 0.95f);
	Expects(duplication < 0.95f);
}

void InstabilitySimulator::close()
{
	parent->close();
}

ConnectionStatus InstabilitySimulator::getStatus() const
{
	return parent->getStatus();
}

void InstabilitySimulator::send(OutboundNetworkPacket&& packet)
{
	auto& rng = Random::getGlobal();

	if (rng.getFloat(0.0f, 1.0f) < packetLoss) {
		// Drop packet
		return;
	}

	do {
		float delay = rng.getFloat(avgLag - lagVariance, avgLag + lagVariance);
		auto now = std::chrono::system_clock::now();
		auto scheduledTime = now + std::chrono::duration_cast<decltype(now)::duration>(std::chrono::duration<double>(delay));
		packets.push(DelayedPacket(scheduledTime, packet));
	} while (rng.getFloat(0.0f, 1.0f) < duplication);

	sendPendingPackets();
}

bool InstabilitySimulator::receive(InboundNetworkPacket& packet)
{
	// This is called every frame, so it's a "good" place for this
	sendPendingPackets();

	return parent->receive(packet);
}

void InstabilitySimulator::sendPendingPackets()
{
	while (!packets.empty() && packets.top().isReady()) {
		OutboundNetworkPacket packet = packets.top().packet;
		parent->send(std::move(packet));
		packets.pop();
	}
}
