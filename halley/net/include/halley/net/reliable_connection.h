#pragma once

#include "iconnection.h"
#include <memory>
#include <vector>

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
	public:
		ReliableConnection(std::shared_ptr<IConnection> parent);

		void close() override;
		ConnectionStatus getStatus() const override;
		void send(NetworkPacket&& packet) override;
		bool receive(NetworkPacket& packet) override;

		void sendTagged(NetworkPacket&& packet, int tag);
		void addAckListener(IReliableConnectionAckListener& listener);
		void removeAckListener(IReliableConnectionAckListener& listener);

	private:
		std::shared_ptr<IConnection> parent;

		unsigned short sequenceSent = 0;
		unsigned short highestReceived = 0xFFFF;

		std::vector<char> receivedSeqs;
		std::vector<char> waitingAcks;
		std::vector<int> tags;

		std::vector<IReliableConnectionAckListener*> ackListeners;

		void internalSend(NetworkPacket& packet, int tag);

		bool processReceivedPacket(NetworkPacket& packet);
		unsigned int generateAckBits();

		void processReceivedAcks(unsigned short ack, unsigned int ackBits);
		void onAckReceived(unsigned short sequence);
	};
}
