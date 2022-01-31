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

void NetworkService::update(Time t)
{
	statsTime += t;
	if (statsTime > 1.0) {
		statsTime -= 1.0;
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

void NetworkService::onSendData(size_t size, size_t nPackets)
{
	sentSize += size;
	sentPackets += nPackets;
}

void NetworkService::onReceiveData(size_t size, size_t nPackets)
{
	receivedSize += size;
	receivedPackets += nPackets;
}

size_t NetworkService::getSentDataPerSecond() const
{
	return lastSentSize;
}

size_t NetworkService::getReceivedDataPerSecond() const
{
	return lastReceivedSize;
}

size_t NetworkService::getSentPacketsPerSecond() const
{
	return lastSentPackets;
}

size_t NetworkService::getReceivedPacketsPerSecond() const
{
	return lastReceivedPackets;
}
