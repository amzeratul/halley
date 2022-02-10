#pragma once

#include <new>
#include <cstddef>
#include <memory>
#include <functional>
#include "message.h"

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

	struct SystemMessageContext {
		int msgId;
		std::unique_ptr<SystemMessage> msg;
		std::function<void(std::byte*)> callback;
	};
}
