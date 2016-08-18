#include "instability_simulator.h"
#include <halley/maths/random.h>
#include <chrono>

using namespace Halley;

InstabilitySimulator::DelayedPacket::DelayedPacket(std::chrono::system_clock::time_point when, NetworkPacket&& packet)
	: when(when)
	, packet(std::move(packet))
{}

bool InstabilitySimulator::DelayedPacket::operator<(const DelayedPacket& other) const
{
	return when > other.when;
}

bool InstabilitySimulator::DelayedPacket::isReady() const
{
	return std::chrono::system_clock::now() >= when;
}

InstabilitySimulator::InstabilitySimulator(std::shared_ptr<IConnection> parent, float avgLag, float lagVariance, float packetLoss)
	: parent(parent)
	, avgLag(avgLag)
	, lagVariance(lagVariance)
	, packetLoss(packetLoss)
{
}

void InstabilitySimulator::close()
{
	parent->close();
}

ConnectionStatus InstabilitySimulator::getStatus() const
{
	return parent->getStatus();
}

void InstabilitySimulator::send(NetworkPacket&& packet)
{
	auto& rng = Random::getGlobal();

	if (rng.getFloat(0.0f, 1.0f) < packetLoss) {
		// Drop packet
		return;
	}

	float delay = rng.getFloat(avgLag - lagVariance, avgLag + lagVariance);
	auto now = std::chrono::system_clock::now();
	auto scheduledTime = now + std::chrono::duration_cast<decltype(now)::duration>(std::chrono::duration<double>(delay));
	packets.push(DelayedPacket(scheduledTime, std::move(packet)));

	sendPendingPackets();
}

bool InstabilitySimulator::receive(NetworkPacket& packet)
{
	// This is called every frame, so it's a "good" place for this
	sendPendingPackets();

	return parent->receive(packet);
}

void InstabilitySimulator::sendPendingPackets()
{
	while (!packets.empty() && packets.top().isReady()) {
		NetworkPacket packet = packets.top().packet;
		parent->send(std::move(packet));
		packets.pop();
	}
}
