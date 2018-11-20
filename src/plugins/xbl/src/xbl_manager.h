#pragma once

#include <memory>
#include <map>
#include <winrt/base.h>
#include <winrt/Windows.Gaming.XboxLive.Storage.h>
#include "halley/data_structures/maybe.h"
#include "halley/utils/utils.h"
#include <halley/core/api/save_data.h>
#include "api/platform_api.h"
#include "halley/concurrency/future.h"

namespace xbox {
	namespace services {
		class xbox_live_context;
		namespace system {
			class xbox_live_user;
		}
	}
}

namespace Halley {
	enum class XBLStatus {
		Disconnected,
		Connecting,
		Connected
	};

	enum class XBLAchievementsStatus {
		Uninitialized,
		Retrieving,
		Ready
	};

	class XBLManager {
	public:
		XBLManager();
		~XBLManager();

		void init();
		void deInit();

		std::shared_ptr<ISaveData> getSaveContainer(const String& name);
		void recreateCloudSaveContainer();

		Maybe<winrt::Windows::Gaming::XboxLive::Storage::GameSaveProvider> getProvider() const;
		XBLStatus getStatus() const;
		Future<AuthTokenResult> getAuthToken(const AuthTokenParameters& parameters);
		void setAchievementProgress(const String& achievementId, int currentProgress, int maximumValue);
		bool isAchievementUnlocked(const String& achievementId, bool defaultValue);

	private:
		std::shared_ptr<xbox::services::system::xbox_live_user> xboxUser;
		std::shared_ptr<xbox::services::xbox_live_context> xboxLiveContext;
		Maybe<winrt::Windows::Gaming::XboxLive::Storage::GameSaveProvider> gameSaveProvider;
		std::map<String, std::shared_ptr<ISaveData>> saveStorage;
		std::map<std::wstring, bool> achievementStatus;
		
		XBLAchievementsStatus achievementsStatus;
		XBLStatus status = XBLStatus::Disconnected;

		void signIn();

		winrt::Windows::Foundation::IAsyncAction getConnectedStorage();
		void retrieveUserAchievementsState();
	};

	class XBLSaveData : public ISaveData {
	public:
		explicit XBLSaveData(XBLManager& manager, String containerName);

		bool isReady() const override;
		Bytes getData(const String& path) override;
		std::vector<String> enumerate(const String& root) override;
		void setData(const String& path, const Bytes& data, bool commit) override;
		void removeData(const String& path) override;
		void commit() override;

		void recreate();
	private:
		bool isSaving;
		XBLManager& manager;
		String containerName;
		mutable Maybe<winrt::Windows::Gaming::XboxLive::Storage::GameSaveContainer> gameSaveContainer;

		void updateContainer() const;
	};
}
