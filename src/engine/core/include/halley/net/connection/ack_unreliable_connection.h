#pragma once

#include "iconnection.h"
#include "halley/concurrency/mutex.h"
#include "halley/data_structures/vector.h"
#include <chrono>

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
		virtual void onPackedLost(uint16_t sequence) = 0;
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

    	[[nodiscard]] size_t getMaxUnreliablePacketSize() const override;
    	[[nodiscard]] size_t getRealMaxUnreliablePacketSize() const override;

        void beginSendUnreliablePackets() override;
        void flushSendUnreliablePackets() override;

        void onUnreliablePacketReceived(gsl::span<const std::byte> packet) override;
        void onUnreliablePacketAck(uint64_t id) override;
        void onUnreliablePacketLost(uint64_t id) override;

        [[nodiscard]] float getLatency() const;

        void setStatsListener(IAckUnreliableConnectionStatsListener* listener);

    private:
    	using Clock = std::chrono::steady_clock;

    	static constexpr size_t headerSize = 12;
    	static constexpr uint8_t headerSignature2[3] = {'h', 'l', 'y'};

    	static constexpr int maxPacketQueueSize = 512;

    	struct SeqIndex
    	{
    		size_t count = 0;

    		void update()
    		{
    			count = count + 1;
    		}

    		[[nodiscard]] uint16_t toSlot() const
    		{
    			return count & 0x7fff;
    		}

    		[[nodiscard]] uint8_t parity() const
    		{
    			return static_cast<uint8_t>((count & 0x78000) >> 15);
    		}

    		[[nodiscard]] bool isInValidRange(SeqIndex other, size_t range) const;

    		static SeqIndex make(uint16_t seqIdx, uint8_t parity);
    	};

        struct SubPacket
        {
            Bytes data;
        	Clock::time_point timestamp;
            size_t dataSize = 0;
            uint16_t seqIdx = 0;
            uint8_t subIdx = 0;
            uint8_t parity = 0;
        	uint8_t resend = 0;
        	bool lost = false;

        	void clear()
        	{
        		seqIdx = 0xffff;
        		// Clear header bytes (keep the signature)
        		constexpr size_t sz = sizeof(headerSignature2);
        		memset(data.data() + sz, 0, headerSize - sz);
        		dataSize = 0;
        	}
        };

        struct InOutQueue
        {
            std::array<SubPacket, maxPacketQueueSize> packets;
        	SeqIndex seqIndex;
            int curPacketIdx = 0;
        	int firstPacketIdx = 0;
        };

        std::shared_ptr<IConnection> parent;
        INetworkServiceStatsListener& networkStatsListener;

        size_t totalMaxPacketSize;
    	size_t realMaxPacketSize;

        Bytes inboundCache;
        InOutQueue inbound;
        InOutQueue outbound;

        std::array<std::pair<int, uint16_t>, maxPacketQueueSize> ackPackets = {};
        int numAckPackets = 0;

    	// Platforms may bring their own tracking of packets ack'd/lost.
    	bool platformSupportsAck = false;

    	// The mean time in seconds, interpolated, how long it took for sent packets to be
    	// acknowledged by the remote peer of this connection.
        float averagePacketAckTime = 1.0f;

        IAckUnreliableConnectionStatsListener* statsListener = nullptr;

    	void close(const std::optional<String>& reason);

        void doSend(gsl::span<const std::byte> packet);
        void doSend(SubPacket& packet, int packetIdx);
    	void doSendUnreliablePacket(gsl::span<const std::byte> packet, uint64_t id) const;

        void doSendAckPackets();
        void onAckPacketsReceive(gsl::span<const std::byte> data, uint8_t parity);
    	bool doProcessAckPacket(SubPacket& slot, int packetIdx, uint16_t seqIdx, uint8_t parity);
    	void forwardOutboundQueue();

    	void resendUnAckPackets(float minResendTimeDiff);

    	void evictInboundQueue(uint16_t seqIdx, uint8_t parity);
    	[[nodiscard]] bool checkOutboundQueue(int numPacketsToSend) const;

    	void setRealMaxPacketSize();

    	static bool isExpiredSeqIndex(const InOutQueue& queue, uint16_t seqIdx, uint8_t parity);
    };

}
