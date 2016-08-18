#pragma once

#include "imessage.h"
#include "imessage_stream.h"
#include <memory>
#include <vector>
#include <map>

namespace Halley
{
	class ReliableConnection;

	class MessageQueue
	{
	public:
		MessageQueue(std::shared_ptr<ReliableConnection> connection);
		
		void addStream(IMessageStream&& stream, int channel);

		std::vector<std::unique_ptr<IMessage>> receiveAll();

		void enqueue(IMessage&& msg, int channel);
		void sendAll();

	private:
		std::shared_ptr<ReliableConnection> connection;
		std::map<int, std::unique_ptr<IMessageStream>> streams;
	};
}
