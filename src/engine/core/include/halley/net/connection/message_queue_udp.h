#pragma once

#include "message_queue.h"
#include "ack_unreliable_connection.h"

namespace Halley
{
    class MessageQueueUDP : public MessageQueue
    {
    public:
        explicit MessageQueueUDP(std::shared_ptr<AckUnreliableConnection> connection);

        [[nodiscard]] bool isConnected() const override;
        void enqueue(OutboundNetworkPacket packet, uint8_t channel) override;
        void sendAll() override;
        Vector<InboundNetworkPacket> receivePackets() override;

        void close();
        [[nodiscard]] ConnectionStatus getStatus() const;
        [[nodiscard]] float getLatency() const;
        [[nodiscard]] size_t getMaxPacketSize() const;

    private:
        struct Outbound {
            OutboundNetworkPacket packet;
            uint8_t channel = 0;
        };

        std::shared_ptr<AckUnreliableConnection> connection;

        Vector<Outbound> outboundQueued;
    };
}
