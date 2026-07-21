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
	};

    class INetworkServiceStatsListener;

    class AckUnreliableConnection : public IConnection, IConnection::IPacketListener
    {
    public:
        explicit AckUnreliableConnection(std::shared_ptr<IConnection> parent, INetworkServiceStatsListener& networkStatsListener);

        void close() override;
        [[nodiscard]] ConnectionStatus getStatus() const override;

        [[nodiscard]] bool isSupported(TransmissionType type) const override;

    	UniqueLock<Mutex> lockSend() override;
        void send(TransmissionType type, OutboundNetworkPacket packet) override;

    	UniqueLock<Mutex> lockReceive() override;
        bool receive(InboundNetworkPacket& packet) override;

    	[[nodiscard]] size_t getMaxUnreliablePacketSize() const override;
    	[[nodiscard]] size_t getRealMaxUnreliablePacketSize() const override;

        void flushSendUnreliablePackets() override;

        void onReceive(gsl::span<const std::byte> packet) override;

        [[nodiscard]] float getLatency() const;

        void setStatsListener(IAckUnreliableConnectionStatsListener* listener);


    private:
    	using Clock = std::chrono::steady_clock;

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

        	void clear()
        	{
        		seqIdx = 0xffff;
        		// Clear header bytes (keep the signature)
        		memset(data.data() + 4, 0, headerSize - 4);
        		dataSize = 0;
        	}
        };

        struct InOutQueue
        {
            std::array<SubPacket, 256 + 1> packets;
        	SeqIndex seqIndex;
            int curPacketIdx = 0;
        	int firstPacketIdx = 0;
        };

        std::shared_ptr<IConnection> parent;
        INetworkServiceStatsListener& networkStatsListener;

        size_t totalMaxPacketSize;
    	size_t realMaxPacketSize;
        static constexpr size_t headerSize = 16;
        static constexpr uint8_t headerSignature2[3] = {'h', 'l', 'y'};

    	// Some platforms may (opt to) call onReceive() from a separate thread.
    	// Need to ensure to not write to the buffers/queues below concurrently.
    	Mutex mutex;

        Bytes inboundCache;
        InOutQueue inbound;
        InOutQueue outbound;

        std::array<std::pair<uint8_t, uint16_t>, 256> ackPackets = {};
        int numAckPackets = 0;

    	// The mean time in seconds, interpolated, how long it took for sent packets to be
    	// acknowledged by the remote peer of this connection.
        float averagePacketAckTime = 1.0f;

        IAckUnreliableConnectionStatsListener* statsListener = nullptr;

    	void close(const std::optional<String>& reason);

    	bool tryCacheSmallPacket(const OutboundNetworkPacket& packet);
    	bool tryReceiveSmallPacket(InboundNetworkPacket& packet);
    	void doFlushSmallPackets();

        void doSend(gsl::span<const std::byte> packet, bool small);
        void doSend(SubPacket& packet, int packetIdx);
    	void doSendUnreliablePacket(gsl::span<const std::byte> packet);

        void doSendAckPackets();
        void onAckPacketsReceive(gsl::span<const std::byte> data, uint8_t parity);
    	void forwardOutboundQueue();

    	void resendUnAckPackets(float minResendTimeDiff);

    	void evictInboundQueue(uint16_t seqIdx, uint8_t parity);
    	bool checkOutboundQueue(int numPacketsToSend) const;

    	static bool isExpiredSeqIndex(const InOutQueue& queue, uint16_t seqIdx, uint8_t parity);
    };

}
