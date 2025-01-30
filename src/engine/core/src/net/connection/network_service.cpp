#include "halley/net/connection/network_service.h"

using namespace Halley;

static const uint64_t handshakeSnd = 0x51662937cc774b87;
static const uint64_t handshakeAck = 0xc3717eacac75d888;

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

void NetworkService::sendHandshake(IConnection& connection)
{
    if (connection.getStatus() == ConnectionStatus::Connecting) {
        std::array<uint8_t, 9> packet = {};
        packet[0] = 0;
        memcpy(&packet[1], &handshakeSnd, 8);
        connection.sendUnreliablePacket(gsl::span((gsl::byte*) packet.data(), 9));
    }
}

void NetworkService::sendHandshakeAccept(IConnection& connection, short id)
{
    Expects(id > 0);

    if (connection.getStatus() == ConnectionStatus::Connecting) {
        std::array<uint8_t, 10> packet = {};
        if (id >= 0x80) {
            packet[0] = 0x80 | (id >> 8);
            packet[1] = id & 0xff;
            memcpy(&packet[2], &handshakeSnd, 8);
            connection.sendUnreliablePacket(gsl::span((gsl::byte*) packet.data(), 10));
        } else {
            packet[0] = id & 0xff;
            memcpy(&packet[1], &handshakeSnd, 8);
            connection.sendUnreliablePacket(gsl::span((gsl::byte*) packet.data(), 9));
        }
    }
}

bool NetworkService::isValidHandshake(gsl::span<const gsl::byte> data, short* connId)
{
    size_t size = data.size();

    if (connId) {
        if (size != 9 && size != 10) {
            return false;
        }
    } else {
        if (size != 8) {
            return false;
        }
    }

    auto packet = (uint8_t*) data.data();

    int id = 0;
    uint64_t payload = 0;

    if (size == 8) {
        memcpy(&payload, packet, 8);
    } else if (size == 9) {
        id = packet[0];
        memcpy(&payload, packet + 1, 8);
    } else {
        id = ((packet[0] & 0x7f) << 8) | packet[1];
        memcpy(&payload, packet + 2, 8);
    }

    if (connId) {
        *connId = (short) id;
    }

    return (payload == handshakeSnd || payload == handshakeAck);
}

short NetworkService::getFreeConnectionId() const
{
    for (short i = 1; i < 1024; i++) {
        if (!hasConnectionWithId(i)) {
            return i;
        }
    }

    throw Exception("Unable to find empty connection id", HalleyExceptions::NetworkPlugin);
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
