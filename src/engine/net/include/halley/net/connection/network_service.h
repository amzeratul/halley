#pragma once
#include <memory>
#include <halley/text/halleystring.h>
#include "iconnection.h"

namespace Halley
{
	enum class IPVersion
	{
		IPv4,
		IPv6
	};

	class NetworkService
	{
	public:
		virtual ~NetworkService() = default;

		virtual void update() = 0;

		virtual void setAcceptingConnections(bool accepting) = 0;
		virtual std::shared_ptr<IConnection> tryAcceptConnection() = 0;
		virtual std::shared_ptr<IConnection> connect(String address, int port) = 0;
	};
}
