#pragma once
#include <memory>

namespace Halley
{
	class NetworkService;

	class NetworkAPI
	{
	public:
		virtual ~NetworkAPI() {}
		virtual std::unique_ptr<NetworkService> createService(int port) = 0;
	};
}
