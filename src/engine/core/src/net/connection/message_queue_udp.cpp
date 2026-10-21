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

    connection->beginSendUnreliablePackets();

    const size_t count = outboundQueued.size();

    size_t idx;
    for (idx = 0; idx < count; idx++) {
        const auto& p = outboundQueued[idx];
        if (!connection->send(IConnection::TransmissionType::Unreliable, p.packet)) {
            break;
        }
    }

    connection->flushSendUnreliablePackets();

    if (idx == count) {
        outboundQueued.clear();
    } else {
        if (outboundQueueSizeLeft == 0) { // Only warns on the first frame, to avoid log spam
            Logger::logWarning("Outbound packet queue full, " + toString(count - idx) + " packets still pending");
        }
        outboundQueued.erase(outboundQueued.begin(), outboundQueued.begin() + static_cast<ptrdiff_t>(idx));
    }

    outboundQueueSizeLeft = count - idx;
    HalleyAssertDev(outboundQueued.size() == outboundQueueSizeLeft);
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
    return connection->getRealMaxUnreliablePacketSize() * 16;
}
