#pragma once

#include "iconnection.h"
#include <memory>
#include <vector>

namespace Halley
{
	class ReliableConnection : public IConnection
	{
	public:
		ReliableConnection(std::shared_ptr<IConnection> parent);

		void close() override;
		ConnectionStatus getStatus() const override;
		void send(NetworkPacket&& packet) override;
		bool receive(NetworkPacket& packet) override;

	private:
		std::shared_ptr<IConnection> parent;

		unsigned short sequenceSent = 0;
		unsigned short highestReceived = 0xFFFF;

		std::vector<char> receivedSeqs;
		std::vector<char> waitingAcks;

		bool processReceivedPacket(NetworkPacket& packet);
		unsigned int generateAckBits();

		void processReceivedAcks(unsigned short ack, unsigned int ackBits);
		void onAckReceived(unsigned short sequence);
	};
}
