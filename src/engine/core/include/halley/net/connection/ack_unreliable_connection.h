#pragma once

#include "iconnection.h"
#include "network_packet.h"
#include <memory>
#include "halley/data_structures/vector.h"
#include <chrono>
#include <deque>
#include <limits>
#include <cstdint>
#include <optional>

namespace Halley
{
	class IAckUnreliableConnectionListener
	{
	public:
		virtual ~IAckUnreliableConnectionListener() = default;

		virtual void onPacketAcked(int tag) = 0;
	};

	class IAckUnreliableConnectionStatsListener
	{
	public:
		virtual ~IAckUnreliableConnectionStatsListener() = default;

		virtual void onPacketSent(uint16_t sequence, size_t size) = 0;
		virtual void onPacketResent(uint16_t sequence) = 0;
		virtual void onPacketAcked(uint16_t sequence) = 0;
		virtual void onPacketReceived(uint16_t sequence, size_t size, bool resend) = 0;
	};

#if 0
	class AckUnreliableSubPacket
	{
	public:
		Vector<gsl::byte> data;
		int tag = -1;
		//bool reliable = false;
		bool resends = false;
		uint16_t seq = std::numeric_limits<uint16_t>::max();
		uint16_t resendSeq = 0;

		AckUnreliableSubPacket()
		{}

		AckUnreliableSubPacket(AckUnreliableSubPacket&& other) = default;

		AckUnreliableSubPacket(Vector<gsl::byte>&& data)
			: data(data)
			, resends(false)
		{}

		AckUnreliableSubPacket(Vector<gsl::byte>&& data, uint16_t resendSeq)
			: data(data)
			, resends(true)
			, resendSeq(resendSeq)
		{}
	};

	class AckUnreliableConnection : public IConnection
	{
		using Clock = std::chrono::steady_clock;

		struct SentPacketData
		{
			Vector<int> tags;
			Clock::time_point timestamp = {};
			bool waiting = false;
		};

	public:
		AckUnreliableConnection(std::shared_ptr<IConnection> parent);

		void close() override;
		[[nodiscard]] ConnectionStatus getStatus() const override;
		[[nodiscard]] bool isSupported(TransmissionType type) const override;
		[[nodiscard]] bool receive(InboundNetworkPacket& packet) override;

		void send(TransmissionType type, OutboundNetworkPacket packet) override;
		Vector<uint16_t> sendTagged(gsl::span<const AckUnreliableSubPacket> subPackets);
		void sendAckPacketsIfNeeded();

		void addAckListener(IAckUnreliableConnectionListener& listener);
		void removeAckListener(IAckUnreliableConnectionListener& listener);

		[[nodiscard]] float getLatency() const { return lag; }
		[[nodiscard]] float getTimeSinceLastSend() const;
		[[nodiscard]] float getTimeSinceLastReceive() const;

		void setStatsListener(IAckUnreliableConnectionStatsListener* listener);

	private:
		std::shared_ptr<IConnection> parent;

		uint16_t nextSequenceToSend = 0;
		uint16_t highestReceived = 0xFFFF;

		Vector<char> receivedSeqs; // 0 = not received, 1 = received
		Vector<SentPacketData> sentPackets;
		std::deque<InboundNetworkPacket> pendingPackets;

		Vector<IAckUnreliableConnectionListener*> ackListeners;
		IAckUnreliableConnectionStatsListener* statsListener = nullptr;

		float lag = 1; // Start at 1 second
		float curLag = 0;

		Clock::time_point lastReceive;
		Clock::time_point lastSend;
		std::optional<Clock::time_point> earliestUnackedMsg;

		void processReceivedPacket(InboundNetworkPacket& packet);
		unsigned int generateAckBits();

		void processReceivedAcks(uint16_t ack, unsigned int ackBits);
		bool onSeqReceived(uint16_t sequence, bool hasSubPacket);
		void onAckReceived(uint16_t sequence);

		void startLatencyReport();
		void reportLatency(float lag);
		void endLatencyReport();

		void notifySend(uint16_t sequence, size_t size);
		void notifyResend(uint16_t sequence);
		void notifyAck(uint16_t sequence);
		void notifyReceive(uint16_t sequence, size_t size, bool resend);
	};
#endif

    class INetworkServiceStatsListener;

    class AckUnreliableConnectionV2 : public IConnection, IConnection::IPacketListener
    {
    public:
        explicit AckUnreliableConnectionV2(std::shared_ptr<IConnection> parent, INetworkServiceStatsListener& networkStatsListener);

        void close() override;
        [[nodiscard]] ConnectionStatus getStatus() const override;

        [[nodiscard]] bool isSupported(TransmissionType type) const override;

        void send(TransmissionType type, OutboundNetworkPacket packet) override;
        bool receive(InboundNetworkPacket& packet) override;

        void onSend(gsl::span<const gsl::byte> packet) override;
        void onReceive(gsl::span<const gsl::byte> packet) override;

        [[nodiscard]] float getLatency() const;

        void setStatsListener(IAckUnreliableConnectionStatsListener* listener);

    private:
        struct SubPacket
        {
            Bytes data;
            size_t dataSize;
            uint16_t seqIdx;
            uint8_t subIdx;
        };

        struct InOutQueue
        {
            std::array<SubPacket, 256> packets;
            int packetIdx = 0;
            uint16_t curSeqIdx = 0;
        };

        std::shared_ptr<IConnection> parent;
        INetworkServiceStatsListener& networkStatsListener;

        size_t maxPacketSize;
        static constexpr size_t headerSize = 16;
        static constexpr uint8_t headerSignature[4] = {'h', 'l', 'y', '0'};

        Bytes inboundCache;
        InOutQueue inbound;
        InOutQueue outbound;

        std::array<uint8_t, 256> ackPackets = {};
        int numAckPackets = 0;

        float lag = 1.0f;

        Vector<IAckUnreliableConnectionListener*> ackListeners;
        IAckUnreliableConnectionStatsListener* statsListener = nullptr;

        void doSend(SubPacket& packet, int packetIdx);

        void doSendAckPackets();
        void onAckPacketsReceive(gsl::span<const gsl::byte> data);
    };

}
