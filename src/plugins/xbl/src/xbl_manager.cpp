#include "xbl_manager.h"
#include "halley/support/logger.h"
#include "halley/text/halleystring.h"
#include "halley/concurrency/concurrent.h"

#include <map>

#include <vccorlib.h>
#include <winrt/base.h>
#include <winrt/Windows.System.UserProfile.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Gaming.XboxLive.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include "xsapi/services.h"
#include <ppltasks.h>
#include <assert.h>

#define GAME_SESSION_TEMPLATE_NAME L"Standard_game_session_without_matchmaking"
#define LOBBY_TEMPLATE_NAME L"test_lobby_invite"

using namespace Halley;

template <typename T>
T from_cx(Platform::Object^ from)
{
	T to{ nullptr };

	winrt::check_hresult(reinterpret_cast<::IUnknown*>(from)
		->QueryInterface(winrt::guid_of<T>(),
			reinterpret_cast<void**>(winrt::put_abi(to))));

	return to;
}

template <typename T>
T^ to_cx(winrt::Windows::Foundation::IUnknown const& from)
{
	return safe_cast<T^>(reinterpret_cast<Platform::Object^>(winrt::get_abi(from)));
}

XBLManager::XBLManager()
{
	multiplayerIncommingInvitationUri = L"";

	multiplayerCurrentSetup.mode = MultiplayerMode::None;
	multiplayerCurrentSetup.key = "";
	multiplayerCurrentSetup.invitationUri = L"";
	multiplayerCurrentSetup.sessionId=-1;

	multiplayerTargetSetup.mode = MultiplayerMode::None;
	multiplayerTargetSetup.key = "";
	multiplayerTargetSetup.invitationUri = L"";
	multiplayerTargetSetup.sessionId=-1;

	multiplayerState = MultiplayerState::NotInitialized;
	multiplayerNextSessionId = 1;
	
	xblMultiplayerManager = nullptr;
	xblMultiplayerContext = 0;

	playerLoggedOut = false;
	loginDelay = 0;
}

XBLManager::~XBLManager()
{
	deInit();
}

void XBLManager::init()
{
	using namespace xbox::services::system;
	signOutHandler = xbox_live_user::add_sign_out_completed_handler([this](const sign_out_completed_event_args&)
	{
		xboxUser.reset();
		xboxLiveContext.reset();
		gameSaveProvider.reset();
		status = XBLStatus::Disconnected;
		achievementsStatus = XBLAchievementsStatus::Uninitialized;
		achievementStatus.clear();
		playerLoggedOut = true;
	});

	signIn();
}

void XBLManager::deInit()
{
	using namespace xbox::services::system;
	xbox_live_user::remove_sign_out_completed_handler(signOutHandler);

	achievementsStatus = XBLAchievementsStatus::Uninitialized;
	achievementStatus.clear();
}

std::shared_ptr<ISaveData> XBLManager::getSaveContainer(const String& name)
{
	auto iter = saveStorage.find(name);
	if (iter == saveStorage.end()) {
		auto save = std::make_shared<XBLSaveData>(*this, name);
		saveStorage[name] = save;
		return save;
	} else {
		return iter->second;
	}
}

void XBLManager::recreateCloudSaveContainer()
{
	if (status == XBLStatus::Connected)
	{
		Concurrent::execute([=]() -> void
		{
			gameSaveProvider.reset();
			status = XBLStatus::Disconnected;
			getConnectedStorage().get();

			std::map<String, std::shared_ptr<ISaveData>>::iterator iter;
			for (iter = saveStorage.begin(); iter != saveStorage.end(); ++iter)
			{
				std::dynamic_pointer_cast<XBLSaveData>(iter->second)->recreate();
			}
		}).get();
	}
}

Maybe<winrt::Windows::Gaming::XboxLive::Storage::GameSaveProvider> XBLManager::getProvider() const
{
	return gameSaveProvider;
}

XBLStatus XBLManager::getStatus() const
{
	return status;
}

class XboxLiveAuthorisationToken : public AuthorisationToken {
public:
	XboxLiveAuthorisationToken(String userId, String token)
	{
		data["userId"] = std::move(userId);
		data["token"] = std::move(token);
	}

	String getType() const override
	{
		return "xboxlive";
	}

	bool isSingleUse() const override
	{
		return false;
	}

	bool isCancellable() const override
	{
		return false;
	}

	void cancel() override
	{	
	}

	std::map<String, String> getMapData() const override
	{
		return data;
	}

private:
	std::map<String, String> data;
	bool playOnline = false;
	bool shareUGC = false;
};

Future<AuthTokenResult> XBLManager::getAuthToken(const AuthTokenParameters& parameters)
{
	Promise<AuthTokenResult> promise;
	if (status == XBLStatus::Connected) {
		auto future = promise.getFuture();

		xboxLiveContext->user()->get_token_and_signature(parameters.method.getUTF16().c_str(), parameters.url.getUTF16().c_str(), parameters.headers.getUTF16().c_str())
			.then([=, promise = std::move(promise)](xbox::services::xbox_live_result<xbox::services::system::token_and_signature_result> result) mutable
		{
			if (result.err()) {
				Logger::logError(result.err_message());
				promise.setValue(AuthTokenRetrievalResult::Error);
			} else {
				auto payload = result.payload();
				auto privileges = String(payload.privileges().c_str());
				auto userId = String(payload.xbox_user_id().c_str());
				auto token = String(payload.token().c_str());

				OnlineCapabilities capabilities;
				for (const auto& priv: privileges.split(' ')) {
					const int privNumber = priv.toInteger();
					if (privNumber == 254) { // MULTIPLAYER_SESSIONS
						capabilities.onlinePlay = true;
					} else if (privNumber == 247) { // USER_CREATED_CONTENT
						capabilities.ugc = true;
					} else if (privNumber == 211) { // SHARE_CONTENT
						capabilities.ugcShare = true;
					} else if (privNumber == 252) { // COMMUNICATIONS
						capabilities.communication = true;
					} else if (privNumber == 249) { // PROFILE_VIEWING
						capabilities.viewProfiles = true;
					}
				}

				promise.setValue(AuthTokenResult(std::make_unique<XboxLiveAuthorisationToken>(userId, token), capabilities));
			}
		});

		return future;
	} else {
		promise.setValue(AuthTokenRetrievalResult::Error);
		return promise.getFuture();
	}
}

void XBLManager::setAchievementProgress(const String& achievementId, int currentProgress, int maximumValue)
{
	if (xboxUser != nullptr && xboxLiveContext != nullptr)
	{
		string_t id (achievementId.cppStr().begin(), achievementId.cppStr().end());
		int progress = (int)floor(((float)currentProgress / (float)maximumValue) * 100.f);
		xboxLiveContext->achievement_service().update_achievement(xboxUser->xbox_user_id(), id, progress).then([=] (xbox::services::xbox_live_result<void> result)
		{ 
			if (result.err())
			{
				Logger::logError(String("Error unlocking achievement '") + achievementId + String("': ") + result.err().value() + " "  + result.err_message());
			}
			else if (progress == 100)
			{
				achievementStatus[id] = true;
			}
		});
	}
}

bool XBLManager::isAchievementUnlocked(const String& achievementId, bool defaultValue)
{
	if (achievementsStatus == XBLAchievementsStatus::Uninitialized)
	{
		Logger::logWarning(String("Trying to get the achievement status before starting the retrieve task!"));
		return false;
	}
	else if (achievementsStatus == XBLAchievementsStatus::Retrieving)
	{
		unsigned long long timeout = GetTickCount64() + 5000;
		while (achievementsStatus == XBLAchievementsStatus::Retrieving && GetTickCount64() < timeout) {}

		if (achievementsStatus == XBLAchievementsStatus::Retrieving)
		{
			Logger::logWarning(String("Achievements are taking too long to load!"));
			return false;
		}
	}

	string_t id(achievementId.cppStr().begin(), achievementId.cppStr().end());
	auto iterator = achievementStatus.find(id);
	if (iterator != achievementStatus.end())
	{
		return iterator->second;
	}
	return defaultValue;
}

String XBLManager::getPlayerName()
{
	if (xboxUser)
	{
		return String(xboxUser->gamertag().c_str());
	}
	return "";
}

bool XBLManager::playerHasLoggedOut()
{
	bool loggedOut = playerLoggedOut;
	if (loggedOut) {
		playerLoggedOut = false;
	}
	return loggedOut;
}

winrt::Windows::Foundation::IAsyncAction XBLManager::onLoggedIn()
{
	xboxLiveContext = std::make_shared<xbox::services::xbox_live_context>(xboxUser);

	retrieveUserAchievementsState();

	co_await getConnectedStorage();
}

void XBLManager::signIn()
{
	xbox::services::system::xbox_live_services_settings::get_singleton_instance()->set_diagnostics_trace_level(xbox::services::xbox_services_diagnostics_trace_level::verbose);
	status = XBLStatus::Connecting;
	
	using namespace xbox::services::system;
	xboxUser = std::make_shared<xbox_live_user>(nullptr);
	auto dispatcher = to_cx<Platform::Object>(winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().Dispatcher());
	
	xboxUser->signin_silently(dispatcher).then([=] (xbox::services::xbox_live_result<sign_in_result> result) -> winrt::Windows::Foundation::IAsyncAction
	{
		if (result.err()) {
			Logger::logError(String("Error signing in to Xbox Live: ") + result.err_message());
			loginDelay = 180;
			status = XBLStatus::Disconnected;
		} else {
			auto resultStatus = result.payload().status();
			switch (resultStatus) {
			case success:
				co_await onLoggedIn();
				break;

			case user_interaction_required:
				xboxUser->signin(dispatcher).then([&](xbox::services::xbox_live_result<sign_in_result> loudResult) -> winrt::Windows::Foundation::IAsyncAction
				{
					if (loudResult.err()) {
						Logger::logError("Error signing in to Xbox live: " + String(loudResult.err_message().c_str()));
						loginDelay = 180;
						status = XBLStatus::Disconnected;
					} else {
						auto resPayload = loudResult.payload();
						switch (resPayload.status()) {
						case success:
							co_await onLoggedIn();
							break;
						default:
							loginDelay = 180;
							status = XBLStatus::Disconnected;
							break;
						}
					}
				}, concurrency::task_continuation_context::use_current());
				break;

			default:
				status = XBLStatus::Disconnected;
			}
		}
	});
}

winrt::Windows::Foundation::IAsyncAction XBLManager::getConnectedStorage()
{
	using namespace winrt::Windows::Gaming::XboxLive::Storage;
	
	auto windowsUser = co_await winrt::Windows::System::User::FindAllAsync();

	GameSaveProviderGetResult result = co_await GameSaveProvider::GetForUserAsync(*windowsUser.First(), xboxLiveContext->application_config()->scid());

	if (result.Status() == GameSaveErrorStatus::Ok) {
		gameSaveProvider = result.Value();
		status = XBLStatus::Connected;
	} else {
		status = XBLStatus::Disconnected;
	}
}

void XBLManager::retrieveUserAchievementsState()
{
	achievementsStatus = XBLAchievementsStatus::Retrieving;
	achievementStatus.clear();

	xboxLiveContext->achievement_service().get_achievements_for_title_id(
		xboxUser->xbox_user_id(),
		xboxLiveContext->application_config()->title_id(),
		xbox::services::achievements::achievement_type::all,
		false,
		xbox::services::achievements::achievement_order_by::title_id,
		0,
		0)
		.then([=](xbox::services::xbox_live_result<xbox::services::achievements::achievements_result> result)
	{
		try
		{
			bool receivedMoreAchievements;
			do
			{
				receivedMoreAchievements = false;
				if (result.err())
				{
					Logger::logError(String("Error retrieving achievements for user '") + xboxUser->gamertag().c_str() + String("': ") + result.err().value() + " " + result.err_message());
					achievementsStatus = XBLAchievementsStatus::Uninitialized;
				}
				else
				{
					std::vector<xbox::services::achievements::achievement> achievements = result.payload().items();
					for (unsigned int i = 0; i < achievements.size(); ++i)
					{
						xbox::services::achievements::achievement achievement = achievements[i];
						bool isAchieved = (achievement.progress_state() == xbox::services::achievements::achievement_progress_state::achieved);
						Logger::logInfo(String("Achievement '") + achievement.name().c_str() + String("' (ID '") + achievement.id().c_str() + String("'): ") + (isAchieved ? String("Achieved") : String("Locked")));
						achievementStatus[achievement.id()] = isAchieved;
					}

					if (result.payload().has_next())
					{
						result = result.payload().get_next(32).get();
						receivedMoreAchievements = true;
					}
					else
					{
						achievementsStatus = XBLAchievementsStatus::Ready;
					}
				}
			} while (receivedMoreAchievements);
		}
		catch (...)
		{
			achievementsStatus = XBLAchievementsStatus::Uninitialized;
			Logger::logError(String("Error retrieving achievements for user '") + xboxUser->gamertag().c_str() + String("'"));
		}
	});
}

void XBLManager::showPlayerInfo(String playerId)
{
	xbox::services::system::title_callable_ui::show_profile_card_ui(playerId.getUTF16());
}

XBLSaveData::XBLSaveData(XBLManager& manager, String containerName)
	: manager(manager)
	, containerName(containerName.isEmpty() ? "save" : containerName)
	, isSaving(false)
{
	updateContainer();
}

void XBLManager::setProfanityCheckForbiddenWordsList(std::vector<String> words)
{
	forbiddenWords.clear();
	forbiddenWords.resize(words.size());
	for (size_t i = 0; i < words.size(); ++i) {
		string_t word = words[i].getUTF16();
		std::transform(word.begin(), word.end(), word.begin(), ::towlower);
		forbiddenWords[i] = String(word.c_str());
	}
}

String XBLManager::performProfanityCheck(String text)
{
	string_t finalText = text.getUTF16();
	string_t lowercaseText = finalText;
	std::transform(lowercaseText.begin(), lowercaseText.end(), lowercaseText.begin(), ::towlower);

	string_t replacement(256, '*');
	for (size_t i = 0; i < forbiddenWords.size(); ++i)
	{
		string_t forbiddenWord = forbiddenWords[i].getUTF16();
		size_t startPos = 0;
		while ((startPos = lowercaseText.find(forbiddenWord, startPos)) != std::wstring::npos) {
			// Replace only full words
			bool validFirstChar = (startPos == 0 || (!isdigit(lowercaseText[startPos - 1]) && !isalpha(lowercaseText[startPos - 1])));
			bool validLastChar = ((startPos + forbiddenWord.length()) >= lowercaseText.length() || (!isdigit(lowercaseText[startPos + forbiddenWord.length()]) && !isalpha(lowercaseText[startPos + forbiddenWord.length()])));

			if (validFirstChar && validLastChar) {
				finalText.replace(startPos, forbiddenWord.length(), replacement, 0, forbiddenWord.length());
			}

			startPos += forbiddenWord.length();
		}
	}

	return String(finalText.c_str());
}

bool XBLSaveData::isReady() const
{
	updateContainer();
	return gameSaveContainer.is_initialized();
}

Bytes XBLSaveData::getData(const String& path)
{
	if (!isReady()) {
		throw Exception("Container is not ready yet!", HalleyExceptions::PlatformPlugin);
	}

	return Concurrent::execute([&] () -> Bytes
	{
		if (isSaving)
		{
			unsigned long long timeout = GetTickCount64() + 3000;
			while (isSaving && GetTickCount64() < timeout) {}

			if (isSaving)
			{
				Logger::logWarning(String("Saving data to connected storage is taking too long!"));
			}
		}

		auto key = winrt::hstring(path.getUTF16());
		std::vector<winrt::hstring> updates;
		updates.push_back(key);
		auto view = winrt::single_threaded_vector(std::move(updates)).GetView();

		auto gameBlob = gameSaveContainer->GetAsync(view).get();

		if (gameBlob.Status() == winrt::Windows::Gaming::XboxLive::Storage::GameSaveErrorStatus::Ok) {
			if (gameBlob.Value().HasKey(key)) {
				auto buffer = gameBlob.Value().Lookup(key);

				auto size = buffer.Length();
				Bytes result(size);
				auto dataReader = winrt::Windows::Storage::Streams::DataReader::FromBuffer(buffer);
				dataReader.ReadBytes(winrt::array_view<uint8_t>(result));

				return result;
			}
		}
		else
		{
			Logger::logError(String("Error getting Blob '") + path + String("': ") + (int)gameBlob.Status());
		}

		return {};
	}).get();
}

std::vector<String> XBLSaveData::enumerate(const String& root)
{
	if (!isReady()) {
		throw Exception("Container is not ready yet!", HalleyExceptions::PlatformPlugin);
	}

	return Concurrent::execute([&] () -> std::vector<String>
	{
		std::vector<String> results;

		auto query = gameSaveContainer->CreateBlobInfoQuery(root.getUTF16().c_str());
		auto info = query.GetBlobInfoAsync().get();
		if (info.Status() == winrt::Windows::Gaming::XboxLive::Storage::GameSaveErrorStatus::Ok) {
			auto& entries = info.Value();
			for (uint32_t i = 0; i < entries.Size(); ++i) {
				results.push_back(String(entries.GetAt(i).Name().c_str()));
			}
		}

		return results;
	}).get();
}

void XBLSaveData::setData(const String& path, const Bytes& data, bool commit)
{
	if (!isReady()) {
		throw Exception("Container is not ready yet!", HalleyExceptions::PlatformPlugin);
	}

	isSaving = true;
	Concurrent::execute([=]() -> void
	{
		auto dataWriter = winrt::Windows::Storage::Streams::DataWriter();
		dataWriter.WriteBytes(winrt::array_view<const uint8_t>(data));

		std::map<winrt::hstring, winrt::Windows::Storage::Streams::IBuffer> updates;
		updates[winrt::hstring(path.getUTF16())] = dataWriter.DetachBuffer();
		auto view = winrt::single_threaded_map(std::move(updates)).GetView();

		auto result = gameSaveContainer->SubmitUpdatesAsync(view, {}, L"").get();
		if (result.Status() != winrt::Windows::Gaming::XboxLive::Storage::GameSaveErrorStatus::Ok)
		{
			Logger::logError(String("Error saving Blob '") + path + String("': ") + (int)result.Status());
		}

		isSaving = false;
	});
}

void XBLSaveData::removeData(const String& path)
{
	if (!isReady()) {
		throw Exception("Container is not ready yet!", HalleyExceptions::PlatformPlugin);
	}

	Concurrent::execute([=]() -> void
	{
		auto key = winrt::hstring(path.getUTF16());
		std::vector<winrt::hstring> updates;
		updates.push_back(key);
		auto view = winrt::single_threaded_vector(std::move(updates)).GetView();

		auto result = gameSaveContainer->SubmitUpdatesAsync({}, view, L"").get();
		if (result.Status() != winrt::Windows::Gaming::XboxLive::Storage::GameSaveErrorStatus::Ok)
		{
			Logger::logError(String("Error deleting Blob '") + path + String("': ") + (int)result.Status());
		}
	}).get();
}

void XBLSaveData::commit()
{
	
}

void XBLSaveData::recreate()
{
	if (manager.getStatus() == XBLStatus::Connected) {
		gameSaveContainer.reset();
		gameSaveContainer = manager.getProvider()->CreateContainer(containerName.getUTF16().c_str());
	}
}

void XBLSaveData::updateContainer() const
{
	if (manager.getStatus() == XBLStatus::Connected) {
		if (!gameSaveContainer) {
			gameSaveContainer = manager.getProvider()->CreateContainer(containerName.getUTF16().c_str());
		}
	} else {
		gameSaveContainer.reset();
	}
}

void XBLManager::update()
{
	if (loginDelay > 0) {
		if (status != XBLStatus::Connecting) {
			--loginDelay;
		}
	} else {
		if ((!xboxUser || !xboxUser->is_signed_in()) && status == XBLStatus::Disconnected) {
			signIn();
		}
	}

	multiplayerUpdate();
}

std::unique_ptr<MultiplayerSession> XBLManager::makeMultiplayerSession(const String& key)
{
	return std::make_unique<XBLMultiplayerSession>(*this, key);
}

void XBLManager::invitationArrived (const std::wstring& uri)
{
	Logger::logInfo(String("Invite received: ") + String(uri.c_str()));
	Concurrent::execute([=]()
	{
		multiplayerIncommingInvitationMutex.lock();

		// Wait until join callback was set
		if (!joinCallback) {
			unsigned long long timeout = GetTickCount64() + 30000;
			while (!joinCallback && GetTickCount64() < timeout) {}
			if (!joinCallback) {
				Logger::logWarning(String("Join callback is taking too long to set!"));
				return;
			}
		}

		// Ensure that save container is ready
		if (!getSaveContainer("")->isReady()) {
			unsigned long long timeout = GetTickCount64() + 30000;
			while (!getSaveContainer("")->isReady() && GetTickCount64() < timeout) {}
			if (!getSaveContainer("")->isReady()) {
				Logger::logWarning(String("Save container is taking too long to be ready!"));
			}
		}

		// Then start multiplayer
		if (multiplayerIncommingInvitationUri != uri && multiplayerTargetSetup.invitationUri != uri && multiplayerCurrentSetup.invitationUri != uri)
		{
			multiplayerIncommingInvitationUri = uri;

			preparingToJoinCallback();
		}
		else
		{
			Logger::logWarning(String("Discarding repeated invite!"));
		}

		multiplayerIncommingInvitationMutex.unlock();
	});
}

bool XBLManager::incommingInvitation ()
{
	return !multiplayerIncommingInvitationUri.empty();
}

int XBLManager::acceptInvitation ()
{
	multiplayerTargetSetup.mode = MultiplayerMode::Invitee;
	multiplayerTargetSetup.key = "";
	multiplayerTargetSetup.invitationUri = multiplayerIncommingInvitationUri;
	multiplayerTargetSetup.sessionId=multiplayerNextSessionId++;

	multiplayerDone();

	multiplayerIncommingInvitationUri = L"";

	return multiplayerTargetSetup.sessionId;
}

int XBLManager::openHost (const String& key)
{
	multiplayerTargetSetup.mode = MultiplayerMode::Inviter;
	multiplayerTargetSetup.key = key;
	multiplayerTargetSetup.invitationUri = L"";
	multiplayerTargetSetup.sessionId=multiplayerNextSessionId++;

	multiplayerDone();

	return multiplayerTargetSetup.sessionId;
}

void XBLManager::showInviteUI()
{
	if (multiplayerState== MultiplayerState::Running) {
		Logger::logDev("NFO: Opening social UI...\n");
		auto result = xblMultiplayerManager->lobby_session()->invite_friends(xboxUser, L"", L"Join my game!!");
		if (result.err()) {
			Logger::logInfo("InviteFriends failed: "+toString(result.err_message().c_str())+"\n");
		}
	}
}

MultiplayerStatus XBLManager::getMultiplayerStatus(int session) const
{
	if ( multiplayerTargetSetup.sessionId!=-1 ) {
		if ( session==-1 || session==multiplayerTargetSetup.sessionId )
			return MultiplayerStatus::Initializing;
		else
			return MultiplayerStatus::Error;
	}
	else {
		if ( multiplayerCurrentSetup.sessionId!=-1 ) {
			if ( session==-1 || session==multiplayerCurrentSetup.sessionId ) {
				switch (multiplayerState) {
					case MultiplayerState::Initializing:
						return MultiplayerStatus::Initializing;

					case MultiplayerState::Running:
						return MultiplayerStatus::Running;

					case MultiplayerState::Error:
						return MultiplayerStatus::Error;

					case MultiplayerState::Ending:
					case MultiplayerState::NotInitialized:
					default:
						return MultiplayerStatus::NotInit; 
				}
			}
			else
				return MultiplayerStatus::Error;
		}
		else {
			if ( session==-1 ) 
				return MultiplayerStatus::NotInit;
			else
				return MultiplayerStatus::Error;
		}
	}

	return MultiplayerStatus::NotInit; 
}

bool XBLManager::isMultiplayerAsHost () const
{
	MultiplayerMode mode = multiplayerTargetSetup.mode;
	if (mode == MultiplayerMode::None) mode=multiplayerCurrentSetup.mode;

	return (mode == MultiplayerMode::Inviter);
}

bool XBLManager::isMultiplayerAsGuest () const
{
	MultiplayerMode mode = multiplayerTargetSetup.mode;
	if (mode == MultiplayerMode::None) mode=multiplayerCurrentSetup.mode;

	return (mode == MultiplayerMode::Invitee);
}

void XBLManager::closeMultiplayer ()
{
	multiplayerDone();
}

void XBLManager::multiplayerUpdate()
{
	switch (multiplayerState) {
		case MultiplayerState::NotInitialized:
			multiplayerUpdate_NotInitialized();
			break;
							
		case MultiplayerState::Initializing:
			multiplayerUpdate_Initializing();
			break;

		case MultiplayerState::Running:
			multiplayerUpdate_Running();
			break;

		case MultiplayerState::Ending:
			multiplayerUpdate_Ending();
			break;
	}

	xblMultiplayerPoolProcess();
}

void XBLManager::multiplayerUpdate_NotInitialized()
{
	if (multiplayerTargetSetup.mode != MultiplayerMode::None) {

		// Get incomming setup
		multiplayerCurrentSetup = multiplayerTargetSetup;

		// Delete incomming setup
		multiplayerTargetSetup.mode = MultiplayerMode::None;
		multiplayerTargetSetup.key = "";
		multiplayerTargetSetup.invitationUri = L"";
		multiplayerTargetSetup.sessionId = -1;

		// Reset operation states
		xblOperation_add_local_user = XBLMPMOperationState::NotRequested;
		xblOperation_set_property = XBLMPMOperationState::NotRequested;
		xblOperation_set_joinability = XBLMPMOperationState::NotRequested;
		xblOperation_join_lobby = XBLMPMOperationState::NotRequested;
		xblOperation_remove_local_user = XBLMPMOperationState::NotRequested;
		
		// MPM Initialization
		Logger::logInfo("NFO: Initialize multiplayer Manager\n");
		xblMultiplayerManager = multiplayer_manager::get_singleton_instance();
		xblMultiplayerManager->initialize(LOBBY_TEMPLATE_NAME);

		// Set state
		multiplayerState = MultiplayerState::Initializing;
	}
}

void XBLManager::multiplayerUpdate_Initializing()
{ 
	switch (multiplayerCurrentSetup.mode) {
		case MultiplayerMode::Inviter:
			multiplayerUpdate_Initializing_Iniviter();
			break;

		case MultiplayerMode::Invitee:
			multiplayerUpdate_Initializing_Inivitee();
			break;
	}
}

void XBLManager::multiplayerUpdate_Initializing_Iniviter()
{
	// Check 'add_local_user' Operation
	if (xblOperation_add_local_user == XBLMPMOperationState::NotRequested) {
		// Add Local User
		Logger::logDev("NFO: Add Local User\n");
		auto result = xblMultiplayerManager->lobby_session()->add_local_user(xboxUser);
		xblOperation_add_local_user = XBLMPMOperationState::Requested;
		if (result.err()) {
			Logger::logError("ERR: Unable to join local user: "+toString(result.err_message().c_str())+"\n" );
			xblOperation_add_local_user = XBLMPMOperationState::Error;
		}
		else {
			Logger::logDev("NFO: Set local user address\n");
			string_t connectionAddress = L"1.1.1.1";
			xblMultiplayerManager->lobby_session()->set_local_member_connection_address(xboxUser, connectionAddress);
		}
	}

	// Check 'set_property' Operation
	if (xblOperation_set_property == XBLMPMOperationState::NotRequested && xblOperation_add_local_user == XBLMPMOperationState::DoneOk) {
		Logger::logDev("NFO: Set server user GameKey property:\n"+toString(multiplayerCurrentSetup.key.c_str()));
		std::string lobbyKey = multiplayerCurrentSetup.key.c_str();
		std::wstring lobbyKeyW (lobbyKey.begin(), lobbyKey.end());
		xblMultiplayerManager->lobby_session()->set_synchronized_properties(L"GameKey", web::json::value::string(lobbyKeyW), (void*)InterlockedIncrement(&xblMultiplayerContext));
		xblOperation_set_property = XBLMPMOperationState::Requested;
	}

	// Check 'set_joinability' Operation
	if (xblOperation_set_joinability == XBLMPMOperationState::NotRequested && xblOperation_add_local_user == XBLMPMOperationState::DoneOk) {
		Logger::logDev("NFO: Set server joinability\n");
		xblMultiplayerManager->set_joinability (joinability::joinable_by_friends, (void*)InterlockedIncrement(&xblMultiplayerContext));
		xblOperation_set_joinability = XBLMPMOperationState::Requested;
	}

	// Check Initialization state based on Operations status
	if (xblOperation_add_local_user == XBLMPMOperationState::DoneOk
	 && xblOperation_set_property == XBLMPMOperationState::DoneOk
	 && xblOperation_set_joinability == XBLMPMOperationState::DoneOk ) {
		// Everthing ok : initaliziation successful
		multiplayerState = MultiplayerState::Running;
	}
	else {
		if (xblOperation_add_local_user == XBLMPMOperationState::Error
		 || xblOperation_set_property == XBLMPMOperationState::Error
		 || xblOperation_set_joinability == XBLMPMOperationState::Error
		) {
			multiplayerState = MultiplayerState::Error;
		}
	}
}

void XBLManager::multiplayerUpdate_Initializing_Inivitee()
{
	// Check 'join_lobby' Operation (aka protocol activation)
	if (xblOperation_join_lobby == XBLMPMOperationState::NotRequested) {
		// Extract handle id from URI
		std::wstring handle = L"";
		size_t pos = multiplayerCurrentSetup.invitationUri.find(L"handle=");
		if (pos != std::string::npos) {
			// Handle id is a hyphenated GUID, so its length is fixed
			handle = multiplayerCurrentSetup.invitationUri.substr(pos + strlen("handle="), 36);
		}
		else {
			Logger::logError("ERR: Unable to extract handle ID from URI: "+toString(multiplayerCurrentSetup.invitationUri.c_str())+"\n");
			xblOperation_join_lobby = XBLMPMOperationState::Error;
			return;
		}

		auto result = xblMultiplayerManager->join_lobby(handle, xboxUser);
		xblOperation_join_lobby = XBLMPMOperationState::Requested;
		if (result.err()) {
			Logger::logError("ERR: Unable to join to lobby: "+toString(result.err_message())+"\n");
			xblOperation_join_lobby = XBLMPMOperationState::Error;
		}
		else {
			Logger::logDev("NFO: Set local user address\n");
			string_t connectionAddress = L"1.1.1.1";
			result = xblMultiplayerManager->lobby_session()->set_local_member_connection_address(xboxUser, connectionAddress, (void*)InterlockedIncrement(&xblMultiplayerContext));
			if (result.err()) {
				Logger::logError("ERR: Unable to set local member connection address: "+toString(result.err_message().c_str())+"\n" );
				xblOperation_join_lobby = XBLMPMOperationState::Error;
			}
		}
	}

	// Check Initialization state based on Operations status
	if (xblOperation_join_lobby == XBLMPMOperationState::DoneOk) {

		// Everthing ok : initaliziation successful
		try {
			// Get game key
			Logger::logDev("NFO: Get server user GameKey property\n");
			auto lobbySession = xblMultiplayerManager->lobby_session();
			web::json::value lobbyJson = lobbySession->properties();
			auto lobbyJsonKey = lobbyJson[L"GameKey"];

			std::wstring lobbyKey = lobbyJsonKey.as_string();
			Logger::logInfo("Got server user GameKey property:" + toString(lobbyKey.c_str()) + "\n");
			multiplayerCurrentSetup.key = String(lobbyKey.c_str());

			// Call join callback
			multiplayerState = MultiplayerState::Running;
			PlatformJoinCallbackParameters params;
			params.param = multiplayerCurrentSetup.key;
			joinCallback(params);

			// Done multiplayer
			multiplayerDone();


		} catch (...) {
			multiplayerState = MultiplayerState::Error;
		}
	}
	else {
		if (xblOperation_join_lobby == XBLMPMOperationState::Error) {
			multiplayerState = MultiplayerState::Error;
		}
	}
}

void XBLManager::multiplayerUpdate_Running()
{
	if (multiplayerCurrentSetup.mode != MultiplayerMode::None) {

		switch (multiplayerCurrentSetup.mode) {
			case MultiplayerMode::Inviter:
				break;

			case MultiplayerMode::Invitee:
				break;
		}
	}
}

void XBLManager::multiplayerUpdate_Ending()
{
	// Check remove user state
	bool opsInProgress = (xblOperation_add_local_user == XBLMPMOperationState::Requested
						 || xblOperation_join_lobby == XBLMPMOperationState::Requested
						 || xblOperation_add_local_user == XBLMPMOperationState::Requested
						 || xblOperation_set_property == XBLMPMOperationState::Requested
						 || xblOperation_set_joinability == XBLMPMOperationState::Requested
						 || xblOperation_join_lobby == XBLMPMOperationState::Requested
						);

	if (!opsInProgress) {
		bool removeUserNeeded = (xblOperation_add_local_user == XBLMPMOperationState::DoneOk
								|| xblOperation_join_lobby == XBLMPMOperationState::DoneOk
								);

		if (removeUserNeeded) {

			// Check 'remove_local_user' Operation
			if ( xblOperation_remove_local_user==XBLMPMOperationState::NotRequested )
			{				
				auto result = xblMultiplayerManager->lobby_session()->remove_local_user(xboxUser);
				xblOperation_remove_local_user=XBLMPMOperationState::Requested;
				if (result.err()) {
					Logger::logError("ERR: Unable to remove local user: "+toString(result.err_message().c_str())+"\n" );
					xblOperation_remove_local_user = XBLMPMOperationState::Error;
				}
			}

			// Check NotInitialized state based on Operations status
			if (  xblOperation_remove_local_user==XBLMPMOperationState::DoneOk
			   || xblOperation_remove_local_user==XBLMPMOperationState::Error
			   )
			{
				// Ending done
				multiplayerState = MultiplayerState::NotInitialized;
			}

		}
		else
		{
			// Ending done
			multiplayerState = MultiplayerState::NotInitialized;
		}
	}
}

void XBLManager::multiplayerDone()
{
	if (multiplayerState != MultiplayerState::NotInitialized) {
		multiplayerState = MultiplayerState::Ending;

		update();
	}
}

void XBLManager::xblMultiplayerPoolProcess()
{
	if (xblMultiplayerManager!=nullptr) {
		std::vector<multiplayer_event> queue = xblMultiplayerManager->do_work();
		for (auto& e : queue) {
			switch (e.event_type()) {

				case multiplayer_event_type::user_added:
					{
						auto userAddedArgs = std::dynamic_pointer_cast<user_added_event_args>(e.event_args());
						if (e.err()) {
							Logger::logError("ERR: event user_added: "+toString(e.err_message().c_str())+"\n");
							xblOperation_add_local_user = XBLMPMOperationState::Error;
						}
						else {
							Logger::logDev("NFO: event user_added ok!...\n");
							xblOperation_add_local_user = XBLMPMOperationState::DoneOk;
						}
					}
					break;

				case multiplayer_event_type::join_lobby_completed:
					{
						auto userAddedArgs = std::dynamic_pointer_cast<user_added_event_args>(e.event_args());
						if (e.err()) {
							Logger::logError("ERR: JoinLobby failed: "+toString(e.err_message().c_str())+"\n" );
							xblOperation_join_lobby = XBLMPMOperationState::Error;
						}
						else {
							Logger::logDev("NFO: JoinLobby ok!...\n");
							xblOperation_join_lobby = XBLMPMOperationState::DoneOk;
						}
					}
					break;

				case multiplayer_event_type::session_property_changed:
					{
						auto gamePropChangedArgs = std::dynamic_pointer_cast<session_property_changed_event_args>(e.event_args());
						if (e.session_type() == multiplayer_session_type::lobby_session) {
							Logger::logDev("NFO: Lobby property changed...\n");
							xblOperation_set_property = XBLMPMOperationState::DoneOk;
						}
						else {
							Logger::logDev("NFO: Game property changed...\n");
						}
					}
					break;

				case multiplayer_event_type::joinability_state_changed:
					{
						if (e.err()) {
							Logger::logError("ERR: Joinabilty change failed: "+toString(e.err_message().c_str())+"\n");
							xblOperation_set_joinability = XBLMPMOperationState::Error;
						}
						else {
							Logger::logDev("NFO: Joinabilty change ok!...\n");
							xblOperation_set_joinability = XBLMPMOperationState::DoneOk;
						}
					}
					break;

				case multiplayer_event_type::user_removed:
					{
						if (e.err()) {
							Logger::logError("ERR: multiplayer_event_type::user_removed failed: "+toString(e.err_message().c_str())+"\n");
							xblOperation_remove_local_user=XBLMPMOperationState::Error;
						}
						else {
							Logger::logDev("NFO: multiplayer_event_type::user_removed ok!...\n");
							xblOperation_remove_local_user=XBLMPMOperationState::DoneOk; 
						}
					}
					break;

				case multiplayer_event_type::join_game_completed:
					Logger::logDev("NFO: multiplayer_event_type::join_game_completed\n");
					break;

				case multiplayer_event_type::member_property_changed:
					Logger::logDev("NFO: multiplayer_event_type::member_property_changed\n");
					break;

				case multiplayer_event_type::member_joined:
					Logger::logDev("NFO: multiplayer_event_type::member_joined\n");
					break;

				case multiplayer_event_type::member_left:
					Logger::logDev("NFO: multiplayer_event_type::member_left\n");
					break;

				case multiplayer_event_type::leave_game_completed:
					Logger::logDev("NFO: multiplayer_event_type::leave_game_completed\n");
					break;

				case multiplayer_event_type::local_member_property_write_completed:
					Logger::logDev("NFO: multiplayer_event_type::local_member_property_write_completed\n");
					break;

				case multiplayer_event_type::local_member_connection_address_write_completed:
					Logger::logDev("NFO: multiplayer_event_type::local_member_connection_address_write_completed\n");
					break;

				case multiplayer_event_type::session_property_write_completed:
					Logger::logDev("NFO: multiplayer_event_type::session_property_write_completed\n");
					break;

				case multiplayer_event_type::session_synchronized_property_write_completed:
					Logger::logDev("NFO: multiplayer_event_type::session_synchronized_property_write_completed\n");
					break;

				case multiplayer_event_type::host_changed:
					Logger::logDev("NFO: multiplayer_event_type::host_changed\n");
					break;

				case multiplayer_event_type::synchronized_host_write_completed:
					Logger::logDev("NFO: multiplayer_event_type::synchronized_host_write_completed\n");
					break;

				case multiplayer_event_type::perform_qos_measurements:
					Logger::logDev("NFO: multiplayer_event_type::perform_qos_measurements\n");
					break;

				case multiplayer_event_type::find_match_completed:
					Logger::logDev("NFO: multiplayer_event_type::find_match_completed\n");
					break;

				case multiplayer_event_type::client_disconnected_from_multiplayer_service:
					Logger::logDev("NFO: multiplayer_event_type::client_disconnected_from_multiplayer_service\n");
					break;

				case multiplayer_event_type::invite_sent:
					Logger::logDev("NFO: multiplayer_event_type::invite_sent\n");
					break;

				case multiplayer_event_type::tournament_registration_state_changed:
					Logger::logDev("NFO: multiplayer_event_type::tournament_registration_state_changed\n");
					break;

				case multiplayer_event_type::tournament_game_session_ready:
					Logger::logDev("NFO: multiplayer_event_type::tournament_game_session_ready\n");
					break;

				case multiplayer_event_type::arbitration_complete:
					Logger::logDev("NFO: multiplayer_event_type::arbitration_complete\n");
					break;

				default:
					Logger::logDev("NFO: multiplayer_event_type::UKNOWN!?!?!\n");
					break;
			}
		}
	}
}

void XBLManager::setJoinCallback(PlatformJoinCallback callback)
{
	joinCallback = callback;
}

void XBLManager::setPreparingToJoinCallback(PlatformPreparingToJoinCallback callback)
{
	preparingToJoinCallback = callback;
}

 XBLMultiplayerSession::XBLMultiplayerSession(XBLManager& manager,const String& key) 
	: manager(manager)
	, key(key)
	, sessionId(-1)
{
	sessionId = manager.openHost ( key );
}

XBLMultiplayerSession::~XBLMultiplayerSession()
{
	manager.closeMultiplayer ();
}

MultiplayerStatus XBLMultiplayerSession::getStatus() const
{
	return manager.getMultiplayerStatus ( sessionId );
}

void XBLMultiplayerSession::showInviteUI()
{
	manager.showInviteUI ();
}

