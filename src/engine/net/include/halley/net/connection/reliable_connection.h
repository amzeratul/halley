#pragma once

#include "iconnection.h"
#include "network_packet.h"
#include <memory>
#include <vector>
#include <chrono>
#include <deque>
#include <limits>

namespace Halley
{
	class IReliableConnectionAckListener
	{
	public:
		virtual ~IReliableConnectionAckListener() {}

		virtual void onPacketAcked(int tag) = 0;
	};

	class ReliableSubPacket
	{
	public:
		std::vector<gsl::byte> data;
		int tag = -1;
		//bool reliable = false;
		bool resends = false;
		unsigned short seq = std::numeric_limits<unsigned short>::max();
		unsigned short resendSeq = 0;

		ReliableSubPacket()
		{}

		ReliableSubPacket(ReliableSubPacket&& other) = default;

		ReliableSubPacket(std::vector<gsl::byte>&& data)
			: data(data)
			, resends(false)
		{}

		ReliableSubPacket(std::vector<gsl::byte>&& data, unsigned short resendSeq)
			: data(data)
			, resends(true)
			, resendSeq(resendSeq)
		{}
	};

	class ReliableConnection : public IConnection
	{
		using Clock = std::chrono::steady_clock;

		struct SentPacketData
		{
			bool waiting = false;
			int tag = -1;
			Clock::time_point timestamp;
		};

	public:
		ReliableConnection(std::shared_ptr<IConnection> parent);

		void close() override;
		ConnectionStatus getStatus() const override;
		bool isSupported(TransmissionType type) const override;
		void send(TransmissionType type, OutboundNetworkPacket packet) override;
		bool receive(InboundNetworkPacket& packet) override;

		void sendTagged(gsl::span<ReliableSubPacket> subPackets);
		void addAckListener(IReliableConnectionAckListener& listener);
		void removeAckListener(IReliableConnectionAckListener& listener);

		float getLatency() const { return lag; }
		float getTimeSinceLastSend() const;
		float getTimeSinceLastReceive() const;

	private:
		std::shared_ptr<IConnection> parent;

		unsigned short nextSequenceToSend = 0;
		unsigned short highestReceived = 0xFFFF;

		std::vector<char> receivedSeqs; // 0 = not received, 1 = received, 2 = received re-send, 3 = both
		std::vector<SentPacketData> sentPackets;
		std::deque<InboundNetworkPacket> pendingPackets;

		std::vector<IReliableConnectionAckListener*> ackListeners;

		float lag = 1; // Start at 1 second
		Clock::time_point lastReceive;
		Clock::time_point lastSend;

		void processReceivedPacket(InboundNetworkPacket& packet);
		unsigned int generateAckBits();

		void processReceivedAcks(unsigned short ack, unsigned int ackBits);
		bool onSeqReceived(unsigned short sequence, bool isResend, unsigned short resendOf);
		void onAckReceived(unsigned short sequence);
		void reportLatency(float lag);
	};
}
