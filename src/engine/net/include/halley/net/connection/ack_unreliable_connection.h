#pragma once

#include "iconnection.h"
#include "network_packet.h"
#include <memory>
#include "halley/data_structures/vector.h"
#include <chrono>
#include <deque>
#include <limits>
#include <cstdint>

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

		uint16_t nextSequenceToSend = 0;
		uint16_t highestReceived = 0xFFFF;

		Vector<char> receivedSeqs; // 0 = not received, 1 = received, 2 = received re-send, 3 = both
		Vector<SentPacketData> sentPackets;
		std::deque<InboundNetworkPacket> pendingPackets;

		Vector<IAckUnreliableConnectionListener*> ackListeners;

		float lag = 1; // Start at 1 second
		Clock::time_point lastReceive;
		Clock::time_point lastSend;

		void processReceivedPacket(InboundNetworkPacket& packet);
		unsigned int generateAckBits();

		void processReceivedAcks(uint16_t ack, unsigned int ackBits);
		bool onSeqReceived(uint16_t sequence, bool isResend, uint16_t resendOf);
		void onAckReceived(uint16_t sequence);
		void reportLatency(float lag);

		void notifySend(uint16_t sequence);
		void notifyResend(uint16_t sequence);
		void notifyAck(uint16_t sequence);
	};
}
