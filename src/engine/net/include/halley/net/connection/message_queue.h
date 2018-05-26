#pragma once

#include "network_message.h"
#include <memory>
#include <vector>
#include <map>

namespace Halley
{	
	struct ChannelSettings
	{
	public:
		ChannelSettings(bool reliable = false, bool ordered = false, bool keepLastSent = false);
		bool reliable;
		bool ordered;
		bool keepLastSent;
	};

	class MessageQueue
	{
	public:
		virtual ~MessageQueue();
		
		template <typename T>
		void addFactory()
		{
			addFactory(std::make_unique<NetworkMessageFactory<T>>());
		}

		virtual void enqueue(std::unique_ptr<NetworkMessage> msg, int channel) = 0;
		virtual void sendAll() = 0;
		virtual std::vector<std::unique_ptr<NetworkMessage>> receiveAll() = 0;

		virtual void setChannel(int channel, ChannelSettings settings);

	protected:
		void addFactory(std::unique_ptr<NetworkMessageFactoryBase> factory);
		int getMessageType(NetworkMessage& msg) const;
		std::unique_ptr<NetworkMessage> deserializeMessage(gsl::span<const gsl::byte> data, unsigned short msgType, unsigned short seq);

	private:
		std::map<std::type_index, int> typeToMsgIndex;
		std::vector<std::unique_ptr<NetworkMessageFactoryBase>> factories;
	};
}
