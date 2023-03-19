#pragma once
#include "halley/support/logger.h"
#include "halley/net/connection/network_message.h"
#include <gsl/gsl>

#include "halley/data_structures/config_node.h"

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
			UpdateInterest,
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
			
			LoggerLevel level;
			String msg;
		};

		class ReloadAssetsMsg final : public DevConMessageBase<MessageType::ReloadAssets>
		{
		public:
			ReloadAssetsMsg() = default;
			ReloadAssetsMsg(Vector<String> assetIds, Vector<String> packIds);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;
			
			Vector<String> assetIds;
			Vector<String> packIds;
		};

		class RegisterInterestMsg final : public DevConMessageBase<MessageType::RegisterInterest>
		{
		public:
			RegisterInterestMsg() = default;
			RegisterInterestMsg(String id, ConfigNode params, uint32_t handle);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;
			
			String id;
			ConfigNode params;
			uint32_t handle;
		};

		class UpdateInterestMsg final : public DevConMessageBase<MessageType::UpdateInterest>
		{
		public:
			UpdateInterestMsg() = default;
			UpdateInterestMsg(uint32_t handle, ConfigNode params);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;
			
			uint32_t handle;
			ConfigNode params;
		};

		class UnregisterInterestMsg final : public DevConMessageBase<MessageType::UnregisterInterest>
		{
		public:
			UnregisterInterestMsg() = default;
			UnregisterInterestMsg(uint32_t handle);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;
			
			uint32_t handle;
		};

		class NotifyInterestMsg final : public DevConMessageBase<MessageType::NotifyInterest>
		{
		public:
			NotifyInterestMsg() = default;
			NotifyInterestMsg(uint32_t handle, ConfigNode data);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;
			
			uint32_t handle;
			ConfigNode data;
		};
	}
}
