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
		namespace multiplayer {
			namespace manager {
				class multiplayer_manager;
			}
		}
	}
}

using namespace xbox::services::multiplayer::manager;

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

		String getPlayerName();

		void showPlayerInfo(String playerId);

		void update();

		void invitationArrived(const std::wstring& uri);
		bool incommingInvitation();
		void acceptInvitation();
		void openHost(const String& key);
		void showInviteUI();
		MultiplayerStatus getMultiplayerStatus() const;
		bool isMultiplayerAsHost() const;
		bool isMultiplayerAsGuest() const;
		void closeMultiplayer();

		void setJoinCallback(PlatformJoinCallback callback);
		void setPreparingToJoinCallback(PlatformPreparingToJoinCallback callback);

	private:
		std::shared_ptr<xbox::services::system::xbox_live_user> xboxUser;
		std::shared_ptr<xbox::services::xbox_live_context> xboxLiveContext;
		Maybe<winrt::Windows::Gaming::XboxLive::Storage::GameSaveProvider> gameSaveProvider;
		std::map<String, std::shared_ptr<ISaveData>> saveStorage;
		std::map<std::wstring, bool> achievementStatus;
		PlatformJoinCallback joinCallback;
		PlatformPreparingToJoinCallback preparingToJoinCallback;
		
		XBLAchievementsStatus achievementsStatus;
		XBLStatus status = XBLStatus::Disconnected;

		void signIn();

		winrt::Windows::Foundation::IAsyncAction getConnectedStorage();
		void retrieveUserAchievementsState();

		enum class MultiplayerMode {
			MultiplayerModeNone, 
			MultiplayerModeInviter, 
			MultiplayerModeInvitee
		};

		enum class MultiplayerState {
			MultiplayerStateNotInitialized,
			MultiplayerStateInitializing,
			MultiplayerStateRunning,
			MultiplayerStateEnding,
			MultiplayerStateError
		};

		enum class XBLMPMOperationState { 
			XBLMPMOperationStateNotRequested,
			XBLMPMOperationStateRequested,
			XBLMPMOperationStateDoneOk,
			XBLMPMOperationStateError
		};

		XBLMPMOperationState    xblOperation_add_local_user;    // INVITER
		XBLMPMOperationState    xblOperation_set_property;      // INVITER
		XBLMPMOperationState    xblOperation_set_joinability;   // INVITER
		XBLMPMOperationState    xblOperation_join_lobby;        // INVITEE

		struct MultiplayerSetup
		{
			MultiplayerMode mode;
			String          key;
			std::wstring    invitationUri;
		};

		std::wstring                            multiplayerIncommingInvitationUri;

		MultiplayerSetup                        multiplayerCurrentSetup;
		MultiplayerSetup                        multiplayerTargetSetup;

		MultiplayerState                        multiplayerState;

		std::shared_ptr<multiplayer_manager>    xblMultiplayerManager;
		uint64_t                                xblMultiplayerContext;

		void                                    multiplayerUpdate();
		void                                    multiplayerUpdate_NotInitialized();
		void                                    multiplayerUpdate_Initializing();
		void                                    multiplayerUpdate_Initializing_Iniviter();
		void                                    multiplayerUpdate_Initializing_Inivitee();
		void                                    multiplayerUpdate_Running();
		void                                    multiplayerUpdate_Ending();

		void                                    multiplayerDone();
		void                                    xblMultiplayerPoolProcess();
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
