#pragma once

#include "message_queue.h"
#include "iconnection.h"

namespace Halley
{	
	class MessageQueueTCP : public MessageQueue
	{
	public:
		MessageQueueTCP(std::shared_ptr<IConnection> connection);
		~MessageQueueTCP() override;

		bool isConnected() const override;
		void enqueue(OutboundNetworkPacket bytes, uint8_t channel) override;
		void sendAll() override;
		Vector<InboundNetworkPacket> receivePackets() override;
				
	private:
		std::shared_ptr<IConnection> connection;
	};
}
