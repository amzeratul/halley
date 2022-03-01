#pragma once
#include "halley/support/logger.h"
#include "halley/net/connection/network_message.h"
#include <gsl/gsl>

namespace Halley
{
	class Serializer;
	class String;
	class MessageQueue;

	namespace DevCon
	{
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

		class LogMsg final : public DevConMessage
		{
		public:
			LogMsg() = default;
			LogMsg(LoggerLevel level, const String& msg);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;

			LoggerLevel getLevel() const;
			const String& getMessage() const;
			MessageType getMessageType() const override;

		private:
			LoggerLevel level;
			String msg;
		};

		class ReloadAssetsMsg final : public DevConMessage
		{
		public:
			ReloadAssetsMsg() = default;
			ReloadAssetsMsg(gsl::span<const String> ids);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;

			Vector<String> getIds() const;

			MessageType getMessageType() const override;

		private:
			Vector<String> ids;
		};
	}
}
