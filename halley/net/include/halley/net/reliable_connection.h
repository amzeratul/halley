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

		bool processReceivedPacket(NetworkPacket& packet);
		void processReceivedAcks(unsigned short ack, unsigned int ackBits);
		unsigned int generateAckBits();
	};
}
