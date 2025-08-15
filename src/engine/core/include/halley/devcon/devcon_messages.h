#pragma once
#include "halley/support/logger.h"
#include "halley/net/connection/network_message.h"
#include <gsl/gsl>

#include "halley/data_structures/config_node.h"
#include "halley/text/i18n.h"

namespace Halley
{
	class Serializer;
	class String;
	class MessageQueue;

	namespace DevCon
	{
		enum class MessageType
		{
			Log,
			ReloadAssets,
			RegisterInterest,
			UpdateInterest,
			UnregisterInterest,
			NotifyInterest,
			SetClientData,
			RPC,
			RPCReply,
			UpdateStrings
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

		class SetClientDataMsg final : public DevConMessageBase<MessageType::SetClientData>
		{
		public:
			SetClientDataMsg() = default;
			SetClientDataMsg(GamePlatform platform, String deviceName, ConfigNode params);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;
			
			GamePlatform platform;
			String deviceName;
			ConfigNode params;
		};

		class RPCMsg final : public DevConMessageBase<MessageType::RPC>
		{
		public:
			RPCMsg() = default;
			RPCMsg(uint64_t id, String method, ConfigNode params);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;
			
			uint64_t id;
			String method;
			ConfigNode params;
		};

		class RPCReplyMsg final : public DevConMessageBase<MessageType::RPCReply>
		{
		public:
			RPCReplyMsg() = default;
			RPCReplyMsg(uint64_t id, ConfigNode result);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;
			
			uint64_t id;
			ConfigNode result;
		};

		class UpdateStringsMsg final : public DevConMessageBase<MessageType::UpdateStrings>
		{
		public:
			UpdateStringsMsg() = default;
			UpdateStringsMsg(I18NLanguage language, HashMap<String, String> strings);

			void serialize(Serializer& s) const override;
			void deserialize(Deserializer& s) override;

			I18NLanguage language;
			HashMap<String, String> strings;
		};
	}
}
