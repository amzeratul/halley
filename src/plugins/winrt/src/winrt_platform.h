#pragma once
#include "halley/core/api/halley_api_internal.h"

namespace Halley
{
	class WinRTSystem;
	class XBLManager;

	class WinRTPlatform : public PlatformAPIInternal
	{
	public:
		explicit WinRTPlatform(WinRTSystem* system);

		void init() override;
		void deInit() override;

		void update() override;

		std::unique_ptr<HTTPRequest> makeHTTPRequest(const String& method, const String& url) override;

		bool canProvideAuthToken() const override;
		Future<AuthTokenResult> getAuthToken(const AuthTokenParameters& parameters) override;

		bool canProvideCloudSave() const override;
		std::shared_ptr<ISaveData> getCloudSaveContainer(const String& containerName) override;

		String getId() override { return "xboxlive"; }
		void setAchievementProgress(const String& achievementId, int currentProgress, int maximumValue) override;
		bool isAchievementUnlocked(const String& achievementId, bool defaultValue) override;

		std::unique_ptr<MultiplayerSession> makeMultiplayerSession(const String& key);

		bool multiplayerProcessingInvitation() override;
		bool multiplayerProcessingInvitationError() override;
		void multiplayerInvitationCancel() override;

		void recreateCloudSaveContainer();

		String getPlayerName() override;

		void invitationArrived(const std::wstring& uri);

		void setJoinCallback(PlatformJoinCallback callback) override;
		void setPreparingToJoinCallback(PlatformPreparingToJoinCallback callback) override;

		bool canShowPlayerInfo() const override { return true; }
		void showPlayerInfo(String playerId) override;

	private:
		std::shared_ptr<XBLManager> xbl;
		WinRTSystem* system;
	};
}
