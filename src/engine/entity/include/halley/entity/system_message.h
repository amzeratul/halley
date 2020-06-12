#pragma once

#include <new>
#include <cstddef>
#include <memory>
#include <functional>

namespace Halley
{
	class SystemMessage
	{
	public:
		virtual ~SystemMessage() {}
		virtual size_t getSize() const = 0;
	};

	struct SystemMessageContext {
		int msgId;
		std::unique_ptr<SystemMessage> msg;
		std::function<void(std::byte*)> callback;
		size_t refCount = 0;
	};
}
