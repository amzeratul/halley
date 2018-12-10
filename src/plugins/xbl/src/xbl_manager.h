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
		bool playerHasLoggedOut();

		void showPlayerInfo(String playerId);

		void update();

		std::unique_ptr<MultiplayerSession> makeMultiplayerSession(const String& key);

		void invitationArrived(const std::wstring& uri);
		bool incommingInvitation();
		int  acceptInvitation();

		int openHost(const String& key);
		MultiplayerStatus getMultiplayerStatus(int session=-1) const;
		bool isMultiplayerAsHost() const;
		bool isMultiplayerAsGuest() const;
		void showInviteUI();
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
		int32_t signOutHandler;
		bool playerLoggedOut;
		
		XBLAchievementsStatus achievementsStatus;
		XBLStatus status = XBLStatus::Disconnected;

		void signIn();

		winrt::Windows::Foundation::IAsyncAction getConnectedStorage();
		winrt::Windows::Foundation::IAsyncAction onLoggedIn();
		void retrieveUserAchievementsState();

		enum class MultiplayerMode {
			None, 
			Inviter, 
			Invitee
		};

		enum class MultiplayerState {
			NotInitialized,
			Initializing,
			Running,
			Ending,
			Error
		};

		enum class XBLMPMOperationState { 
			NotRequested,
			Requested,
			DoneOk,
			Error
		};

		XBLMPMOperationState xblOperation_add_local_user;    // INVITER
		XBLMPMOperationState xblOperation_set_property;      // INVITER
		XBLMPMOperationState xblOperation_set_joinability;   // INVITER
		XBLMPMOperationState xblOperation_join_lobby;        // INVITEE
		XBLMPMOperationState xblOperation_remove_local_user; // INVITER & INVITEE

		struct MultiplayerSetup {
			MultiplayerMode mode;
			String key;
			std::wstring invitationUri;
			int	sessionId;
		};

		std::wstring multiplayerIncommingInvitationUri;

		MultiplayerSetup multiplayerCurrentSetup;
		MultiplayerSetup multiplayerTargetSetup;

		MultiplayerState multiplayerState;
		int	multiplayerNextSessionId;

		std::shared_ptr<multiplayer_manager> xblMultiplayerManager;
		uint64_t xblMultiplayerContext;

		void multiplayerUpdate();
		void multiplayerUpdate_NotInitialized();
		void multiplayerUpdate_Initializing();
		void multiplayerUpdate_Initializing_Iniviter();
		void multiplayerUpdate_Initializing_Inivitee();
		void multiplayerUpdate_Running();
		void multiplayerUpdate_Ending();

		void multiplayerDone();
		void xblMultiplayerPoolProcess();
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

	class XBLMultiplayerSession : public MultiplayerSession {
	public:
		explicit XBLMultiplayerSession(XBLManager& manager,const String& key);
		virtual ~XBLMultiplayerSession();

		MultiplayerStatus getStatus() const override;
		void showInviteUI() override;
	private:
		XBLManager& manager;
		String key;
		int sessionId;
	};
}
