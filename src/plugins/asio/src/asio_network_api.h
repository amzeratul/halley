#pragma once
#include "halley/api/halley_api_internal.h"

namespace Halley
{
	class AsioNetworkAPI : public NetworkAPIInternal
	{
	public:
		std::unique_ptr<NetworkService> createService(NetworkProtocol protocol, int port) override;
		void init() override;
		void deInit() override;
	};

    class AsioPlatformAPI : public PlatformAPIInternal
    {
    public:
        explicit AsioPlatformAPI(String playerName);

        void init() override;
        void deInit() override;

        String getId() override;
        void update() override;

        String getPlayerName() override;
        String getAccountId() override;

        bool canProvideAuthToken() const override;
        Future<AuthTokenResult> getAuthToken(const Halley::AuthTokenParameters& parameters) override;

        std::shared_ptr<NetworkService> createNetworkService(uint16_t port) override;

    private:
        String playerName;
    };
}
