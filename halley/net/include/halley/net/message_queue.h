#pragma once

#include "network_message.h"
#include "imessage_stream.h"
#include <memory>
#include <vector>
#include <map>
#include "reliable_connection.h"
#include <list>
#include <chrono>

namespace Halley
{
	class ReliableConnection;

	class MessageQueue : private IReliableConnectionAckListener
	{
		struct PendingPacket
		{
			std::vector<std::unique_ptr<NetworkMessage>> msgs;
			std::chrono::steady_clock::time_point timeSent;
		};

	public:
		MessageQueue(std::shared_ptr<ReliableConnection> connection);
		~MessageQueue();
		
		void addStream(std::unique_ptr<IMessageStream> stream, int channel);

		std::vector<std::unique_ptr<NetworkMessage>> receiveAll();

		void enqueue(std::unique_ptr<NetworkMessage> msg, int channel);
		void sendAll();

	private:
		std::shared_ptr<ReliableConnection> connection;
		std::map<int, std::unique_ptr<IMessageStream>> channels;

		std::list<std::unique_ptr<NetworkMessage>> pendingMsgs;
		std::map<int, PendingPacket> pendingPackets;
		int nextPacketId = 0;

		void onPacketAcked(int tag) override;
		void checkReSend();
	};
}
