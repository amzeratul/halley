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
			ReloadAssets,
			RegisterInterest,
			UnregisterInterest,
			NotifyInterest
		};

		class DevConMessage : public NetworkMessage
		{
		public:
			virtual MessageType getMessageType() const = 0;
		};

		template <MessageType _msgType>
		class DevConMessageBase : public DevConMessage
		{
		public:
			constexpr static MessageType msgType = _msgType;

			uint16_t getNetworkIndex() const override
			{
				return getTypeIndex();
			}

			constexpr static uint16_t getTypeIndex()
			{
				return static_cast<uint16_t>(msgType);
			}

			MessageType getMessageType() const override
			{
				return msgType;
			}
		};

		class LogMsg final : public DevConMessageBase<MessageType::Log>
		{
		public:
			LogMsg() = default;
			LogMsg(LoggerLevel level, const String& msg);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;

			LoggerLevel getLevel() const;
			const String& getMessage() const;

		private:
			LoggerLevel level;
			String msg;
		};

		class ReloadAssetsMsg final : public DevConMessageBase<MessageType::ReloadAssets>
		{
		public:
			ReloadAssetsMsg() = default;
			ReloadAssetsMsg(gsl::span<const String> ids);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;

			Vector<String> getIds() const;

		private:
			Vector<String> ids;
		};
	}
}
