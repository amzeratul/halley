#include "halley/net/connection/instability_simulator.h"
#include <halley/maths/random.h>
#include <chrono>

using namespace Halley;

InstabilitySimulator::DelayedPacket::DelayedPacket(std::chrono::system_clock::time_point when, TransmissionType type, OutboundNetworkPacket packet, uint64_t id)
	: when(when)
	, type(type)
	, packet(std::move(packet))
	, id(id)
{}

bool InstabilitySimulator::DelayedPacket::operator<(const DelayedPacket& other) const
{
	return when > other.when;
}

bool InstabilitySimulator::DelayedPacket::isReady() const
{
	return std::chrono::system_clock::now() >= when;
}

InstabilitySimulator::InstabilitySimulator(std::shared_ptr<IConnection> parent)
	: parent(std::move(parent))
	, avgLag(0.0f)
	, lagVariance(0.0f)
	, packetLoss(0.0f)
	, duplication(0.0f)
{
}

void InstabilitySimulator::setLag(float average, float variance)
{
	avgLag = average;
	lagVariance = variance;
	checkActive();
}

void InstabilitySimulator::setPacketLoss(float chance)
{
	packetLoss = std::clamp(chance, 0.0f, 0.95f);
	checkActive();
}

void InstabilitySimulator::setDuplication(float chance)
{
	duplication = std::clamp(chance, 0.0f, 0.95f);
	checkActive();
}

void InstabilitySimulator::close()
{
	parent->close();
}

ConnectionStatus InstabilitySimulator::getStatus() const
{
	return parent->getStatus();
}

bool InstabilitySimulator::isSupported(TransmissionType type) const
{
	return parent->isSupported(type);
}

void InstabilitySimulator::send(TransmissionType type, OutboundNetworkPacket packet)
{
	if (!active) {
		parent->send(type, packet);
		return;
	}

	auto& rng = Random::getGlobal();

	if (rng.getFloat(0.0f, 1.0f) < packetLoss) {
		// Drop packet
		return;
	}

	do {
		float delay = rng.getFloat(avgLag - lagVariance, avgLag + lagVariance);
		auto now = std::chrono::system_clock::now();
		auto scheduledTime = now + std::chrono::duration_cast<decltype(now)::duration>(std::chrono::duration<double>(delay));
		packets.emplace(scheduledTime, type, packet, 0);
	} while (rng.getFloat(0.0f, 1.0f) < duplication);

	sendPendingPackets();
}

bool InstabilitySimulator::receive(InboundNetworkPacket& packet)
{
	// This is called every frame, so it's a "good" place for this
	sendPendingPackets();

	return parent->receive(packet);
}

size_t InstabilitySimulator::getMaxUnreliablePacketSize() const
{
	return parent->getMaxUnreliablePacketSize();
}

void InstabilitySimulator::onConnect(short connId)
{
	return parent->onConnect(connId);
}

void InstabilitySimulator::sendUnreliablePacket(gsl::span<const std::byte> packet, uint64_t id)
{
	if (!active) {
		parent->sendUnreliablePacket(packet, id);
		return;
	}

	HalleyAssertDev(getMaxUnreliablePacketSize() > 0);

	auto& rng = Random::getGlobal();

	if (rng.getFloat(0.0f, 1.0f) < packetLoss) {
		// Drop packet
		return;
	}

	do {
		float delay = rng.getFloat(avgLag - lagVariance, avgLag + lagVariance);
		auto now = std::chrono::system_clock::now();
		auto scheduledTime = now + std::chrono::duration_cast<decltype(now)::duration>(std::chrono::duration<double>(delay));
		packets.emplace(scheduledTime, TransmissionType::Unreliable, OutboundNetworkPacket(packet), id);
	} while (rng.getFloat(0.0f, 1.0f) < duplication);

	sendPendingPackets();
}

void InstabilitySimulator::setUnreliablePacketListener(IPacketListener* listener)
{
	parent->setUnreliablePacketListener(listener);
}

void InstabilitySimulator::sendPendingPackets()
{
	while (!packets.empty() && packets.top().isReady()) {
		const auto& head = packets.top();
		OutboundNetworkPacket packet = head.packet;

		if (head.type != TransmissionType::Unreliable || parent->getMaxUnreliablePacketSize() == 0) {
			parent->send(head.type, std::move(packet));
		} else {
			parent->sendUnreliablePacket(packet.getBytes(), head.id);
		}

		packets.pop();
	}
}

void InstabilitySimulator::checkActive()
{
	active = avgLag > 0.0f || packetLoss > 0.0f || duplication > 0.0f;
}
