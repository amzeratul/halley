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
        explicit AsioPlatformAPI(String playerName, const std::optional<String>& joinLobbyAddress);

        void init() override;
        void deInit() override;

        String getId() override;
        void update() override;

        String getPlayerName() override;
        String getAccountId() override;

        bool canProvideAuthToken() const override;
        Future<AuthTokenResult> getAuthToken(const AuthTokenParameters& parameters) override;

    	void showBrowseGamesToJoinUI() override;
    	void setJoinCallback(PlatformJoinCallback callback) override;
    	void setPreparingToJoinCallback(PlatformPreparingToJoinCallback callback) override;

    	std::shared_ptr<NetworkService> createNetworkService(uint16_t port) override;

    private:
        String playerName;
    	std::optional<String> joinLobbyAddress;

    	PlatformJoinCallback joinCallback;
    	PlatformPreparingToJoinCallback preparingToJoinCallback;

    	bool preparingInvitation = false;
    	bool readyInvitation = false;
    };
}
