#pragma once

#include "network_message.h"
#include <memory>
#include "halley/data_structures/vector.h"
#include <map>

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

	class MessageQueue
	{
	public:
		virtual ~MessageQueue();
		
		template <typename T>
		void addFactory()
		{
			addFactory(std::make_unique<NetworkMessageFactory<T>>());
		}

		virtual bool isConnected() const = 0;
		virtual void enqueue(std::unique_ptr<NetworkMessage> msg, int channel) = 0;
		virtual void sendAll() = 0;
		virtual Vector<std::unique_ptr<NetworkMessage>> receiveAll() = 0;

		virtual void setChannel(int channel, ChannelSettings settings);

	protected:
		void addFactory(std::unique_ptr<NetworkMessageFactoryBase> factory);
		uint16_t getMessageType(NetworkMessage& msg) const;
		std::unique_ptr<NetworkMessage> deserializeMessage(gsl::span<const gsl::byte> data, uint16_t msgType, uint16_t seq);

	private:
		std::map<std::type_index, uint16_t> typeToMsgIndex;
		Vector<std::unique_ptr<NetworkMessageFactoryBase>> factories;
	};
}
