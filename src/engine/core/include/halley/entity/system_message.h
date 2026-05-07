#pragma once

#include <new>
#include <cstddef>
#include <memory>
#include <functional>
#include "message.h"
#include "halley/utils/utils.h"
#include "halley/text/enum_names.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/concurrency/future.h"

namespace Halley
{
	enum class SystemMessageDestinationType : uint8_t {
		Local,
		Host,
		AllClients,
		RemoteClients,
		SpecificPeer
	};

	template <>
	struct EnumNames<SystemMessageDestinationType> {
		constexpr auto operator()() const {
			return std::to_array({
				"local",
				"host",
				"allClients",
				"remoteClients",
				"specificPeer"
			});
		}
	};

	class SystemMessageDestination {
	public:
		using enum SystemMessageDestinationType;

		SystemMessageDestinationType type;
		uint8_t dstPeerId;

		constexpr SystemMessageDestination(SystemMessageDestinationType type = Local, uint8_t dstPeerId = 0)
			: type(type)
			, dstPeerId(dstPeerId)
		{
		}

		SystemMessageDestination(const ConfigNode& node);

		constexpr bool operator==(const SystemMessageDestination& other) const = default;
		constexpr bool operator!=(const SystemMessageDestination& other) const = default;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		String toString() const;
		ConfigNode toConfigNode() const;
	};
	
	class SystemMessage : public Message
	{
	public:
		virtual SystemMessageDestination getMessageDestination() const = 0;
	};

	using SystemMessageCallback = std::function<void(std::byte*, Bytes)>;

	struct SystemMessageContext {
		int msgId;
		bool remote;
		std::unique_ptr<SystemMessage> msg;
		SystemMessageCallback callback;

		SystemMessageContext(std::unique_ptr<SystemMessage> msg = {}, SystemMessageCallback callback = {}, bool remote = false)
			: msgId(msg ? msg->getId() : 0)
			, remote(remote)
			, msg(std::move(msg))
			, callback(std::move(callback))
		{}

		template <typename T>
		void setResult(T& result) const
		{
			if (!callback) {
				return;
			}

			if constexpr (std::is_same_v<T, VoidWrapper>) {
				callback(nullptr, {});
			} else {
		        if (remote) {
		            callback(nullptr, Serializer::toBytes(result, SerializerOptions(SerializerOptions::maxVersion)));
		        } else {
		            callback(reinterpret_cast<std::byte*>(&result), {});
		        }
			}
		}

		template <typename T>
		void setResult(Future<T>& futureResult) const
		{
			if (!callback) {
				return;
			}

			futureResult.then([callback = callback, remote = remote] (T result) {
				if (remote) {
		            callback(nullptr, Serializer::toBytes(result, SerializerOptions(SerializerOptions::maxVersion)));
		        } else {
		            callback(reinterpret_cast<std::byte*>(&result), {});
		        }
			});
		}
	};
}
