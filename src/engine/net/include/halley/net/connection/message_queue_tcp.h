#pragma once

#include "message_queue.h"
#include "iconnection.h"

namespace Halley
{	
	class MessageQueueTCP : public MessageQueue
	{
	public:
		MessageQueueTCP(std::shared_ptr<IConnection> connection);

		bool isConnected() const override;
		void enqueue(std::unique_ptr<NetworkMessage> msg, int channel) override;
		void sendAll() override;
		std::vector<std::unique_ptr<NetworkMessage>> receiveAll() override;
				
	private:
		std::shared_ptr<IConnection> connection;
	};
}
