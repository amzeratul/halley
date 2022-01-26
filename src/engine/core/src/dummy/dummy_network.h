#pragma once
#include "api/halley_api_internal.h"
#include "halley/net/connection/network_service.h"

namespace Halley {
	class DummyNetworkAPI : public NetworkAPIInternal
	{
	public:
		void init() override;
		void deInit() override;

		std::unique_ptr<NetworkService> createService(NetworkProtocol protocol, int port) override;
	};

	class DummyNetworkService : public NetworkService
	{
	public:
		void update() override;
		String startListening(AcceptCallback callback) override;
		void stopListening() override;
		std::shared_ptr<IConnection> connect(const String& address) override;
	};
}
