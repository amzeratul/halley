#pragma once

#include "iconnection.h"
#include "network_packet.h"
#include <memory>
#include <vector>
#include <chrono>
#include <deque>

namespace Halley
{
	class IReliableConnectionAckListener
	{
	public:
		virtual ~IReliableConnectionAckListener() {}

		virtual void onPacketAcked(int tag) = 0;
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
		void send(OutboundNetworkPacket&& packet) override;
		bool receive(InboundNetworkPacket& packet) override;

		void sendTagged(OutboundNetworkPacket&& packet, int tag);
		void addAckListener(IReliableConnectionAckListener& listener);
		void removeAckListener(IReliableConnectionAckListener& listener);

		float getLatency() const { return lag; }
		float getTimeSinceLastSend() const;
		float getTimeSinceLastReceive() const;

	private:
		std::shared_ptr<IConnection> parent;

		unsigned short sequenceSent = 0;
		unsigned short highestReceived = 0xFFFF;

		std::vector<char> receivedSeqs; // 0 = not received, 1 = received, 2 = received re-send, 3 = both
		std::vector<SentPacketData> sentPackets;
		std::deque<InboundNetworkPacket> pendingPackets;

		std::vector<IReliableConnectionAckListener*> ackListeners;

		float lag = 0;
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
