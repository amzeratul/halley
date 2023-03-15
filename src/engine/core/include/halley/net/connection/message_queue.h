#pragma once

#include "network_message.h"
#include <memory>
#include "halley/data_structures/vector.h"
#include <map>

#include "network_packet.h"

namespace Halley
{	
	struct ChannelSettings
	{
		constexpr ChannelSettings(bool reliable = false, bool ordered = false, bool keepLastSent = false)
			: reliable(reliable)
			, ordered(ordered)
			, keepLastSent(keepLastSent)
		{}

		bool reliable;
		bool ordered;
		bool keepLastSent;
	};

	class MessageQueue : public NetworkMessageFactories
	{
	public:
		virtual ~MessageQueue();
		
		virtual bool isConnected() const = 0;
		virtual void enqueue(OutboundNetworkPacket packet, uint8_t channel) = 0;
		virtual void sendAll() = 0;
		virtual Vector<InboundNetworkPacket> receivePackets() = 0;

		void enqueue(std::unique_ptr<NetworkMessage> msg, uint8_t channel);
		Vector<std::unique_ptr<NetworkMessage>> receiveMessages();

		virtual void setChannel(uint8_t channel, ChannelSettings settings);
	};
}
