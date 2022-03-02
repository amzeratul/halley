#pragma once

#include "network_message.h"
#include <memory>
#include "halley/data_structures/vector.h"
#include "ack_unreliable_connection.h"
#include <map>
#include <list>
#include <chrono>
#include "message_queue.h"
#include <cstdint>

namespace Halley
{
	class AckUnreliableConnection;

	class MessageQueueUDP : public MessageQueue, private IAckUnreliableConnectionListener
	{
		struct Inbound {
			InboundNetworkPacket packet;
			uint16_t seq;
			uint8_t channel;
		};

		struct Outbound {
			OutboundNetworkPacket packet;
			uint16_t seq;
			uint8_t channel;
		};

		struct PendingPacket
		{
			Vector<Outbound> msgs;
			std::chrono::steady_clock::time_point timeSent;
			size_t size;
			uint16_t seq;
			bool reliable;
		};

		struct Channel
		{
			Vector<Inbound> receiveQueue;
			uint16_t lastAckSeq = 0;
			uint16_t lastSentSeq = 0;
			uint16_t lastReceivedSeq = 0;
			ChannelSettings settings;
			bool initialized = false;

			void getReadyMessages(Vector<InboundNetworkPacket>& out);
		};

	public:
		MessageQueueUDP(std::shared_ptr<AckUnreliableConnection> connection);
		~MessageQueueUDP();
		
		void setChannel(uint8_t channel, ChannelSettings settings) override;

		Vector<InboundNetworkPacket> receivePackets() override;

		void enqueue(OutboundNetworkPacket packet, uint8_t channel) override;
		void sendAll() override;

	private:
		std::shared_ptr<AckUnreliableConnection> connection;
		Vector<Channel> channels;

		std::list<Outbound> outboundQueued;
		std::map<int, PendingPacket> pendingPackets;
		int nextPacketId = 0;

		void onPacketAcked(int tag) override;
		void checkReSend(Vector<AckUnreliableSubPacket>& collect);

		AckUnreliableSubPacket createPacket();
		AckUnreliableSubPacket makeTaggedPacket(Vector<Outbound>& msgs, size_t size, bool resends = false, uint16_t resendSeq = 0);
		Vector<gsl::byte> serializeMessages(const Vector<Outbound>& msgs, size_t size) const;

		void receiveMessages();
	};
}
