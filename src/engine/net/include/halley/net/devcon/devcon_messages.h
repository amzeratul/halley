#pragma once
#include "halley/net/connection/message_queue.h"
#include "halley/support/logger.h"

namespace Halley
{
	class Serializer;
	class String;

	namespace DevCon
	{
		constexpr static int devConPort = 12500;
		
		void setupMessageQueue(MessageQueue& queue);

		
		enum class MessageType
		{
			Log,
			ReloadAssets
		};


		class DevConMessage : public NetworkMessage
		{
		public:
			virtual ~DevConMessage() = default;
			virtual MessageType getMessageType() const = 0;
		};

		class LogMsg : public DevConMessage
		{
		public:
			LogMsg(gsl::span<const gsl::byte> data);
			LogMsg(LoggerLevel level, const String& msg);

			void serialize(Serializer& s) const override;

			LoggerLevel getLevel() const;
			const String& getMessage() const;
			MessageType getMessageType() const override;

		private:
			LoggerLevel level;
			String msg;
		};
	}
}
