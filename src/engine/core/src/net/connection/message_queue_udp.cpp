#include "halley/net/connection/message_queue_udp.h"

using namespace Halley;

MessageQueueUDP::MessageQueueUDP(std::shared_ptr<AckUnreliableConnection> connection)
    : connection(std::move(connection))
{

}

bool MessageQueueUDP::isConnected() const
{
    return connection->getStatus() == ConnectionStatus::Connected;
}

void MessageQueueUDP::enqueue(OutboundNetworkPacket packet, uint8_t channel)
{
    Outbound p = {packet, channel};
    outboundQueued.emplace_back(p);
}

void MessageQueueUDP::sendAll()
{
    if (connection->getStatus() != ConnectionStatus::Connected) {
        return;
    }

    for (auto& p : outboundQueued) {
        connection->send(IConnection::TransmissionType::Unreliable, p.packet);
    }

    connection->flushOutboundQueue();

    outboundQueued.clear();
}

Vector<InboundNetworkPacket> MessageQueueUDP::receivePackets()
{
    Vector<InboundNetworkPacket> packetsIn;

    InboundNetworkPacket packet;
    while (connection->receive(packet)) {
        packetsIn.push_back(std::move(packet));
    }

    return packetsIn;
}

void MessageQueueUDP::close()
{
    connection->close();
}

ConnectionStatus MessageQueueUDP::getStatus() const
{
    return connection->getStatus();
}

float MessageQueueUDP::getLatency() const
{
    return connection->getLatency();
}

size_t MessageQueueUDP::getMaxPacketSize() const
{
    // AckUnreliableConnection can split up packets into up to 16*MTU
    return connection->getMaxUnreliablePacketSize() * 16;
}
