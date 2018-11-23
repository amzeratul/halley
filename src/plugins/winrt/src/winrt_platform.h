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

		void recreateCloudSaveContainer();

		bool canShowPlayerName() const override { return true; };
		String getPlayerName() override;

	private:
		std::shared_ptr<XBLManager> xbl;
		WinRTSystem* system;
	};
}
