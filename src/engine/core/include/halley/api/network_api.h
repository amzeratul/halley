#pragma once
#include <memory>

namespace Halley
{
	class NetworkService;

	enum class NetworkProtocol
	{
		TCP,
		UDP
	};

	class NetworkAPI
	{
	public:
		virtual ~NetworkAPI() {}
		virtual std::unique_ptr<NetworkService> createService(NetworkProtocol protocol, int port = 0) = 0;
	};
}
