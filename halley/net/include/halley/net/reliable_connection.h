#pragma once

#include "iconnection.h"
#include <memory>
#include <vector>
#include <chrono>

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
		struct SentPacketData
		{
			bool waiting = false;
			int tag = -1;
			std::chrono::system_clock::time_point timestamp;
		};

	public:
		ReliableConnection(std::shared_ptr<IConnection> parent);

		void close() override;
		ConnectionStatus getStatus() const override;
		void send(NetworkPacket&& packet) override;
		bool receive(NetworkPacket& packet) override;

		void sendTagged(NetworkPacket&& packet, int tag);
		void addAckListener(IReliableConnectionAckListener& listener);
		void removeAckListener(IReliableConnectionAckListener& listener);

		float getLatency() const { return lag; }
		float getTimeSinceLastSend() const;
		float getTimeSinceLastReceive() const;

	private:
		std::shared_ptr<IConnection> parent;

		unsigned short sequenceSent = 0;
		unsigned short highestReceived = 0xFFFF;

		std::vector<char> receivedSeqs;
		std::vector<SentPacketData> sentPackets;

		std::vector<IReliableConnectionAckListener*> ackListeners;

		float lag = 0;
		std::chrono::system_clock::time_point lastReceive;
		std::chrono::system_clock::time_point lastSend;

		void internalSend(NetworkPacket& packet, int tag);

		bool processReceivedPacket(NetworkPacket& packet);
		unsigned int generateAckBits();

		void processReceivedAcks(unsigned short ack, unsigned int ackBits);
		void onAckReceived(unsigned short sequence);
		void reportLatency(float lag);
	};
}
