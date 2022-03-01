#pragma once

#include "iconnection.h"
#include "network_packet.h"
#include <memory>
#include "halley/data_structures/vector.h"
#include <chrono>
#include <deque>
#include <limits>

namespace Halley
{
	class IAckUnreliableConnectionListener
	{
	public:
		virtual ~IAckUnreliableConnectionListener() {}

		virtual void onPacketAcked(int tag) = 0;
	};

	class AckUnreliableSubPacket
	{
	public:
		Vector<gsl::byte> data;
		int tag = -1;
		//bool reliable = false;
		bool resends = false;
		unsigned short seq = std::numeric_limits<unsigned short>::max();
		unsigned short resendSeq = 0;

		AckUnreliableSubPacket()
		{}

		AckUnreliableSubPacket(AckUnreliableSubPacket&& other) = default;

		AckUnreliableSubPacket(Vector<gsl::byte>&& data)
			: data(data)
			, resends(false)
		{}

		AckUnreliableSubPacket(Vector<gsl::byte>&& data, unsigned short resendSeq)
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
			bool waiting = false;
			int tag = -1;
			Clock::time_point timestamp;
		};

	public:
		AckUnreliableConnection(std::shared_ptr<IConnection> parent);

		void close() override;
		ConnectionStatus getStatus() const override;
		bool isSupported(TransmissionType type) const override;
		void send(TransmissionType type, OutboundNetworkPacket packet) override;
		bool receive(InboundNetworkPacket& packet) override;

		void sendTagged(gsl::span<AckUnreliableSubPacket> subPackets);
		void addAckListener(IAckUnreliableConnectionListener& listener);
		void removeAckListener(IAckUnreliableConnectionListener& listener);

		float getLatency() const { return lag; }
		float getTimeSinceLastSend() const;
		float getTimeSinceLastReceive() const;

	private:
		std::shared_ptr<IConnection> parent;

		unsigned short nextSequenceToSend = 0;
		unsigned short highestReceived = 0xFFFF;

		Vector<char> receivedSeqs; // 0 = not received, 1 = received, 2 = received re-send, 3 = both
		Vector<SentPacketData> sentPackets;
		std::deque<InboundNetworkPacket> pendingPackets;

		Vector<IAckUnreliableConnectionListener*> ackListeners;

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
