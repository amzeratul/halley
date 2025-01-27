#pragma once

#include "iconnection.h"
#include "network_packet.h"
#include "halley/data_structures/vector.h"

namespace Halley
{
	class IAckUnreliableConnectionStatsListener
	{
	public:
		virtual ~IAckUnreliableConnectionStatsListener() = default;

		virtual void onPacketSent(uint16_t sequence, size_t size) = 0;
		virtual void onPacketResent(uint16_t sequence) = 0;
		virtual void onPacketAcked(uint16_t sequence) = 0;
		virtual void onPacketReceived(uint16_t sequence, size_t size, bool resend) = 0;
	};

    class INetworkServiceStatsListener;

    class AckUnreliableConnection : public IConnection, IConnection::IPacketListener
    {
    public:
        explicit AckUnreliableConnection(std::shared_ptr<IConnection> parent, INetworkServiceStatsListener& networkStatsListener);

        void close() override;
        [[nodiscard]] ConnectionStatus getStatus() const override;

        [[nodiscard]] bool isSupported(TransmissionType type) const override;

        void send(TransmissionType type, OutboundNetworkPacket packet) override;
        bool receive(InboundNetworkPacket& packet) override;

        void onSend(gsl::span<const gsl::byte> packet) override;
        void onReceive(gsl::span<const gsl::byte> packet) override;

        [[nodiscard]] float getLatency() const;

        void setStatsListener(IAckUnreliableConnectionStatsListener* listener);

    	void flushOutboundQueue();

    private:
    	using Clock = std::chrono::steady_clock;

        struct SubPacket
        {
            Bytes data;
            size_t dataSize = 0;
            uint16_t seqIdx = 0;
            uint8_t subIdx = 0;
        	Clock::time_point timestamp;
        };

        struct InOutQueue
        {
            std::array<SubPacket, 256 + 1> packets;
            int curPacketIdx = 0;
        	int firstPacketIdx = 0;
            uint16_t curSeqIdx = 0x7e00;
        };

        std::shared_ptr<IConnection> parent;
        INetworkServiceStatsListener& networkStatsListener;

        size_t maxPacketSize;
        static constexpr size_t headerSize = 16;
        static constexpr uint8_t headerSignature[4] = {'h', 'l', 'y', '0'};

        Bytes inboundCache;
        InOutQueue inbound;
        InOutQueue outbound;

        std::array<std::pair<uint8_t, uint16_t>, 256> ackPackets = {};
        int numAckPackets = 0;

        float lag = 1.0f;

    	float simulatePacketLoss = 0.0f;

        IAckUnreliableConnectionStatsListener* statsListener = nullptr;

    	void close(const std::optional<String>& reason);

    	bool tryCacheSmallPacket(const OutboundNetworkPacket& packet);
    	bool tryReceiveSmallPacket(InboundNetworkPacket& packet);
    	void doFlushSmallPackets();

        void doSend(gsl::span<const gsl::byte> packet, bool small);
        void doSend(SubPacket& packet, int packetIdx);
    	void doSendUnreliablePacket(gsl::span<const gsl::byte> packet, uint16_t seqIdx);

        void doSendAckPackets();
        void onAckPacketsReceive(gsl::span<const gsl::byte> data);

    	void resendUnAckPackets(float minResendTimeDiff);

    	static bool isExpiredSeqIndex(const InOutQueue& queue, uint16_t seqIdx);
    };

}
