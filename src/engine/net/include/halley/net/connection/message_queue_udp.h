#pragma once

#include "network_message.h"
#include <memory>
#include <vector>
#include <map>
#include "reliable_connection.h"
#include <list>
#include <chrono>
#include "message_queue.h"

namespace Halley
{
	class ReliableConnection;

	class MessageQueueUDP : public MessageQueue, private IReliableConnectionAckListener
	{
		struct PendingPacket
		{
			std::vector<std::unique_ptr<NetworkMessage>> msgs;
			std::chrono::steady_clock::time_point timeSent;
			size_t size;
			unsigned short seq;
			bool reliable;
		};

		struct Channel
		{
			std::vector<std::unique_ptr<NetworkMessage>> receiveQueue;
			std::unique_ptr<NetworkMessage> lastAck;
			unsigned short lastAckSeq = 0;
			unsigned short lastSentSeq = 0;
			unsigned short lastReceivedSeq = 0;
			ChannelSettings settings;
			bool initialized = false;

			void getReadyMessages(std::vector<std::unique_ptr<NetworkMessage>>& out);
		};

	public:
		MessageQueueUDP(std::shared_ptr<ReliableConnection> connection);
		~MessageQueueUDP();
		
		void setChannel(int channel, ChannelSettings settings) override;

		std::vector<std::unique_ptr<NetworkMessage>> receiveAll() override;

		void enqueue(std::unique_ptr<NetworkMessage> msg, int channel) override;
		void sendAll() override;

	private:
		std::shared_ptr<ReliableConnection> connection;
		std::vector<Channel> channels;

		std::list<std::unique_ptr<NetworkMessage>> pendingMsgs;
		std::map<int, PendingPacket> pendingPackets;
		int nextPacketId = 0;

		void onPacketAcked(int tag) override;
		void checkReSend(std::vector<ReliableSubPacket>& collect);

		ReliableSubPacket createPacket();
		ReliableSubPacket makeTaggedPacket(std::vector<std::unique_ptr<NetworkMessage>>& msgs, size_t size, bool resends = false, unsigned short resendSeq = 0);
		std::vector<gsl::byte> serializeMessages(const std::vector<std::unique_ptr<NetworkMessage>>& msgs, size_t size) const;

		void receiveMessages();
	};
}
