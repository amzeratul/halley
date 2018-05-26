#pragma once
#include "halley/net/connection/network_service.h"

namespace Halley
{
	class AsioTCPNetworkService : public NetworkService
	{
	public:
		AsioTCPNetworkService(int port);

		void update() override;
		void setAcceptingConnections(bool accepting) override;
		std::shared_ptr<IConnection> tryAcceptConnection() override;
		std::shared_ptr<IConnection> connect(String address, int port) override;
	};
}
