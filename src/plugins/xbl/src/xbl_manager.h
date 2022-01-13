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

	class XBLManager;

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

	class XBLMPMOperationStateCtrl { 
	public:

		XBLMPMOperationStateCtrl();

		void reset();

		void setStateRequested();
		void setStateError();
		void setStateDoneOk();

		bool checkStateNotRequested();
		bool checkStateRequested();
		bool checkStateError();
		bool checkStateDoneOk();

		void enableTimeout( bool active );
		void renewTimeoutTime();

	private:

		enum class OpState { 
			NotRequested,
			Requested,
			DoneOk,
			Error
		};
		 
		OpState getState() const;

		mutable OpState state;
		mutable bool timeOutActive;
		mutable ULONGLONG requestStartTime;
	};

	class XBLManager {
	public:
		XBLManager();
		~XBLManager();

		void init();
		void deInit();

		Future<PlatformSignInResult> signIn();
		bool isSignedIn() const;

		std::shared_ptr<ISaveData> getSaveContainer(const String& name);
		void recreateCloudSaveContainer();

		std::optional<winrt::Windows::Gaming::XboxLive::Storage::GameSaveProvider> getProvider() const;
		XBLStatus getStatus() const;
		Future<AuthTokenResult> getAuthToken(const AuthTokenParameters& parameters);
		void setAchievementProgress(const String& achievementId, int currentProgress, int maximumValue);
		bool isAchievementUnlocked(const String& achievementId, bool defaultValue);

		String getPlayerName();
		bool playerHasLoggedOut();

		void showPlayerInfo(String playerId);

		void update();

		std::unique_ptr<MultiplayerLobby> makeMultiplayerLobby(const String& key);

		void invitationArrived(const std::wstring& uri);
		bool incommingInvitation();
		int  acceptInvitation();

		int openHost(const String& key);
		MultiplayerStatus getMultiplayerStatus(int session=-1) const;
		bool isMultiplayerAsHost() const;
		bool isMultiplayerAsGuest() const;
		void showInviteUI();
		void closeMultiplayer(bool deepReset, int session = -1);

		void setJoinCallback(PlatformJoinCallback callback);
		void setPreparingToJoinCallback(PlatformPreparingToJoinCallback callback);
		void setJoinErrorCallback(PlatformJoinErrorCallback callback);

		void setProfanityCheckForbiddenWordsList(std::vector<String> words);
		String performProfanityCheck(String text);

		void suspend();
		void resume();
		bool isSuspended() const;

	private:
		
		bool suspended;

		std::shared_ptr<xbox::services::system::xbox_live_user> xboxUser;
		std::shared_ptr<xbox::services::xbox_live_context> xboxLiveContext;
		std::optional<winrt::Windows::Gaming::XboxLive::Storage::GameSaveProvider> gameSaveProvider;
		std::map<String, std::shared_ptr<ISaveData>> saveStorage;
		std::map<std::wstring, bool> achievementStatus;
		PlatformJoinCallback joinCallback;
		PlatformPreparingToJoinCallback preparingToJoinCallback;
		PlatformJoinErrorCallback joinErrorCallback;
		int32_t signOutHandler;
		bool playerLoggedOut;
		
		std::vector<String> forbiddenWords;
		XBLAchievementsStatus achievementsStatus;
		XBLStatus status = XBLStatus::Disconnected;

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

		XBLMPMOperationStateCtrl xblOperation_add_local_user;    // INVITER
		XBLMPMOperationStateCtrl xblOperation_set_property;      // INVITER
		XBLMPMOperationStateCtrl xblOperation_set_joinability;   // INVITER
		XBLMPMOperationStateCtrl xblOperation_join_lobby;        // INVITEE
		XBLMPMOperationStateCtrl xblOperation_remove_local_user; // INVITER & INVITEE

		struct MultiplayerSetup {
			MultiplayerMode mode;
			String key;
			std::wstring invitationUri;
			int	sessionId;
		};

		std::wstring multiplayerIncommingInvitationUri;
		std::mutex multiplayerIncommingInvitationMutex;

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

		void multiplayeEnableTimeout( bool active );
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
		mutable std::optional<winrt::Windows::Gaming::XboxLive::Storage::GameSaveContainer> gameSaveContainer;

		void updateContainer() const;
	};

	class XBLMultiplayerLobby : public MultiplayerLobby {
	public:
		explicit XBLMultiplayerLobby(XBLManager& manager,const String& key);
		virtual ~XBLMultiplayerLobby();

		MultiplayerStatus getStatus() const override;
		void showInviteUI(int maxPlayers, const std::map<I18NLanguage, String>& messagePerLanguage) override;
	private:
		XBLManager& manager;
		String key;
		int sessionId;
	};
}
