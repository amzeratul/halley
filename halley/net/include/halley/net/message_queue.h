#pragma once

#include "network_message.h"
#include <memory>
#include <vector>
#include <map>
#include "reliable_connection.h"
#include <list>
#include <chrono>

namespace Halley
{
	class ReliableConnection;

	struct ChannelSettings
	{
	public:
		ChannelSettings(bool reliable = false, bool ordered = false, bool keepLastSent = false);
		bool reliable;
		bool ordered;
		bool keepLastSent;
	};
	
	class MessageQueue : private IReliableConnectionAckListener
	{
		struct PendingPacket
		{
			std::vector<std::unique_ptr<NetworkMessage>> msgs;
			std::chrono::steady_clock::time_point timeSent;
		};

		struct Channel
		{
			std::unique_ptr<NetworkMessage> lastSent;
			ChannelSettings settings;
		};

	public:
		MessageQueue(std::shared_ptr<ReliableConnection> connection);
		~MessageQueue();
		
		void setChannel(int channel, ChannelSettings settings);

		std::vector<std::unique_ptr<NetworkMessage>> receiveAll();

		void enqueue(std::unique_ptr<NetworkMessage> msg, int channel);
		void sendAll();

	private:
		std::shared_ptr<ReliableConnection> connection;
		std::map<int, Channel> channels;

		std::list<std::unique_ptr<NetworkMessage>> pendingMsgs;
		std::map<int, PendingPacket> pendingPackets;
		int nextPacketId = 0;

		void onPacketAcked(int tag) override;
		void checkReSend();
	};
}
