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
			size_t size;
			unsigned short seq;
			bool reliable;
		};

		struct Channel
		{
			std::vector<std::unique_ptr<NetworkMessage>> receiveQueue;
			std::unique_ptr<NetworkMessage> lastAck;
			unsigned int lastAckSeq = 0;
			unsigned int lastSeq = 0;
			unsigned short lastReceived = 0;
			ChannelSettings settings;
			bool initialized = false;

			void getReadyMessages(std::vector<std::unique_ptr<NetworkMessage>>& out);
		};

	public:
		MessageQueue(std::shared_ptr<ReliableConnection> connection);
		~MessageQueue();
		
		void setChannel(int channel, ChannelSettings settings);

		template <typename T>
		void addFactory()
		{
			addFactory(std::make_unique<NetworkMessageFactory<T>>());
		}

		std::vector<std::unique_ptr<NetworkMessage>> receiveAll();

		void enqueue(std::unique_ptr<NetworkMessage> msg, int channel);
		void sendAll();

	private:
		std::shared_ptr<ReliableConnection> connection;
		std::vector<Channel> channels;

		std::map<std::type_index, int> typeToMsgIndex;
		std::vector<std::unique_ptr<NetworkMessageFactoryBase>> factories;

		std::list<std::unique_ptr<NetworkMessage>> pendingMsgs;
		std::map<int, PendingPacket> pendingPackets;
		int nextPacketId = 0;

		void onPacketAcked(int tag) override;
		void checkReSend(std::vector<ReliableSubPacket>& collect);

		ReliableSubPacket createPacket();
		ReliableSubPacket makeTaggedPacket(std::vector<std::unique_ptr<NetworkMessage>>& msgs, size_t size, bool resends = false, unsigned short resendSeq = 0);
		std::vector<gsl::byte> serializeMessages(const std::vector<std::unique_ptr<NetworkMessage>>& msgs, size_t size) const;

		void receiveMessages();
		std::unique_ptr<NetworkMessage> deserializeMessage(gsl::span<const gsl::byte> data, unsigned short msgType, unsigned short seq);

		void addFactory(std::unique_ptr<NetworkMessageFactoryBase> factory);
		int getMessageType(NetworkMessage& msg) const;
	};
}
