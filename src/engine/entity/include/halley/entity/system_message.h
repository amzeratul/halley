#pragma once

#include <new>
#include <cstddef>
#include <memory>
#include <functional>
#include "message.h"
#include "halley/utils/utils.h"

namespace Halley
{
	enum class SystemMessageDestination {
		Local,
		Host,
		AllClients,
		RemoteClients
	};

	template <>
	struct EnumNames<SystemMessageDestination> {
		constexpr std::array<const char*, 4> operator()() const {
			return {{
				"local",
				"host",
				"allClients",
				"remoteClients"
			}};
		}
	};
	
	class SystemMessage : public Message
	{
	};

	using SystemMessageCallback = std::function<void(std::byte*, Bytes)>;

	struct SystemMessageContext {
		int msgId;
		std::unique_ptr<SystemMessage> msg;
		SystemMessageCallback callback;
	};
}
