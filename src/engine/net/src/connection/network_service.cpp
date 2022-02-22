#include "connection/network_service.h"

using namespace Halley;

std::shared_ptr<IConnection> NetworkService::Acceptor::accept()
{
	if (!choiceMade) {
		choiceMade = true;
		return doAccept();
	} else {
		return {};
	}
}

void NetworkService::Acceptor::reject()
{
	if (!choiceMade) {
		choiceMade = true;
		doReject();
	}
}

void NetworkService::Acceptor::ensureChoiceMade()
{
	reject();
}

void NetworkServiceWithStats::onUpdateStats()
{
}

Time NetworkServiceWithStats::getStatUpdateInterval() const
{
	return 1.0;
}

void NetworkServiceWithStats::update(Time t)
{
	statsTime += t;
	if (statsTime > getStatUpdateInterval()) {
		statsTime = 0;

		onUpdateStats();
		
		lastSentSize = sentSize;
		lastReceivedSize = receivedSize;
		lastSentPackets = sentPackets;
		lastReceivedPackets = receivedPackets;
		sentSize = 0;
		receivedSize = 0;
		sentPackets = 0;
		receivedPackets = 0;
	}
}

void NetworkServiceWithStats::onSendData(size_t size, size_t nPackets)
{
	sentSize += size;
	sentPackets += nPackets;
}

void NetworkServiceWithStats::onReceiveData(size_t size, size_t nPackets)
{
	receivedSize += size;
	receivedPackets += nPackets;
}

size_t NetworkServiceWithStats::getSentDataPerSecond() const
{
	return lastSentSize;
}

size_t NetworkServiceWithStats::getReceivedDataPerSecond() const
{
	return lastReceivedSize;
}

size_t NetworkServiceWithStats::getSentPacketsPerSecond() const
{
	return lastSentPackets;
}

size_t NetworkServiceWithStats::getReceivedPacketsPerSecond() const
{
	return lastReceivedPackets;
}
