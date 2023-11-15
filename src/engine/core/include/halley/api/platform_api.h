#pragma once

#include <memory>
#include <map>
#include <halley/concurrency/future.h>
#include <halley/utils/utils.h>
#include "halley/text/i18n.h"
#include "halley/text/enum_names.h"

namespace Halley
{
	class ISaveData;
	class String;
	class InputKeyboard;
	class NetworkService;

	class AuthorisationToken {
	public:
		virtual ~AuthorisationToken() {}

		virtual String getType() const = 0;
		virtual bool isSingleUse() const = 0;
		virtual bool isCancellable() const = 0;

		virtual void cancel() = 0;
		virtual std::map<String, String> getMapData() const { return {}; }
	};
	
	enum class AuthTokenRetrievalResult {
		OK,
		Cancelled,
		ErrorAlreadyReported,
		Error,
		Unknown
	};

	struct OnlineCapabilities {
		bool onlinePlay = false;
		bool ugc = false;
		bool ugcShare = false;
		bool communication = false;
		bool viewProfiles = false;

		OnlineCapabilities() {}
		
		void setAll(bool value)
		{
			onlinePlay = value;
			ugc = value;
			ugcShare = value;
			communication = value;
			viewProfiles = value;
		}
		
		bool includes(const OnlineCapabilities& other)
		{
			auto checkInclusion = [](bool has, bool otherHas)
			{
				return otherHas ? has : true;
			};
			return checkInclusion(onlinePlay, other.onlinePlay)
				&& checkInclusion(ugc, other.ugc)
				&& checkInclusion(communication, other.communication)
				&& checkInclusion(viewProfiles, other.viewProfiles);
		}
	};

	struct AuthTokenResult {
		AuthTokenRetrievalResult result = AuthTokenRetrievalResult::Unknown;
		std::unique_ptr<AuthorisationToken> token;
		OnlineCapabilities capabilitiesSupported;

		AuthTokenResult(AuthTokenRetrievalResult result)
			: result(result)
		{}

		AuthTokenResult(AuthTokenRetrievalResult result, OnlineCapabilities capabilities)
			: result(result)
			, capabilitiesSupported(capabilities)
		{}

		AuthTokenResult(std::unique_ptr<AuthorisationToken> token, OnlineCapabilities capabilities)
			: result(AuthTokenRetrievalResult::OK)
			, token(std::move(token))
			, capabilitiesSupported(capabilities)
		{}
	};

	struct AuthTokenParameters {
		String url;
		String method;
		String headers;
		OnlineCapabilities capabilitiesRequired;
	};

	enum class MultiplayerStatus {
		NotInit,
		Initializing,
		Running,
		Error,
		Unsupported
	};

	enum class MultiplayerPrivacy {
		Private,
		FriendsOnly,
		Public
	};

	template <>
	struct EnumNames<MultiplayerPrivacy> {
		constexpr std::array<const char*, 3> operator()() const {
			return { {
					"private",
					"friends_only",
					"public"
				} };
		}
	};

	class MultiplayerLobby {
	public:
		virtual ~MultiplayerLobby() = default;
		virtual MultiplayerStatus getStatus() const = 0;
		virtual void showInviteUI(int maxPlayers, const std::map<I18NLanguage, String>& messagePerLanguage) = 0;
		virtual void setPrivacy(MultiplayerPrivacy privacy) { }
	};

	// This is the join callback, see PlatformAPI's method for more details
	struct PlatformJoinCallbackParameters {
		// Fill these as necessary
		String param;
	};
	using PlatformJoinCallback = std::function<void(PlatformJoinCallbackParameters)>;
	using PlatformPreparingToJoinCallback = std::function<void(void)>;
	using PlatformJoinErrorCallback = std::function<void(void)>;

	struct PlatformSignInResult {
		bool success = false;
		bool canRetry = false;
		float retryDelaySeconds = 0;

		PlatformSignInResult() = default;

		PlatformSignInResult(bool success, bool canRetry, float retryDelaySeconds)
			: success(success)
			, canRetry(canRetry)
			, retryDelaySeconds(retryDelaySeconds)
		{}
	};

	class PlatformAPI
	{
	public:
		virtual ~PlatformAPI() {}

		virtual String getId() = 0; // Platform id
		virtual void update() = 0;
		
		
		//////////////////////////
		// Login & authentication

		virtual bool needsSignIn() const { return false; }
		virtual Future<PlatformSignInResult> signIn()
		{
			return Future<PlatformSignInResult>::makeImmediate(PlatformSignInResult(true, false, 0));
		}
		virtual bool isSignedIn() const { return true; }
		virtual bool playerHasLoggedOut() { return false; }

		virtual bool canProvideAuthToken() const = 0;
		virtual Future<AuthTokenResult> getAuthToken(const AuthTokenParameters& parameters) = 0;

		virtual String getPlayerName() { return "Player"; }
		virtual String getUniquePlayerIdString() { return ""; } // This has to be a valid filename


		//////////////
		// Cloud save

		virtual bool canProvideCloudSave() const { return false; }
		virtual std::shared_ptr<ISaveData> getCloudSaveContainer(const String& containerName = "") { return {}; }

		
		////////////////////////
		// Stats & Achievements

		virtual void setAchievementProgress(const String& achievementId, int currentProgress, int maximumValue) {} // Complete when currentProgress == maximumValue
		virtual bool isAchievementUnlocked(const String& achievementId, bool defaultValue) { return defaultValue; }
		virtual bool isAchievementSystemReady() const { return true; }
		virtual bool mustUnlockAchievementsOnUserAction() const { return false; }
		virtual void setStat(const String& statId, int value) {}
		virtual int getStatInt(const String& statId) { return 0; }
		virtual void incrementStat(const String& statId) { setStat(statId, getStatInt(statId) + 1); }
		virtual bool isStatsSystemReady() const { return true; }
		virtual void resetAllStatsAndAchievements() {}


		/////////////
		// UGC & DLC

		virtual bool hasOfflineUGCCapabilities() { return true; } // Some platforms require custom handling when missing UGC capabilities
		virtual bool handleMissingUGCCapabilities() { return false; } // Returns true if has handled ugc access errors

		virtual bool hasDLC(const String& key) const { return false; }
		virtual Future<bool> requestGetDLC(const String& key)
		{
			Promise<bool> result;
			result.setValue(false);
			return result.getFuture();
		}

		///////////////////
		// Lobbies/invites

		// Return empty unique_ptr if not supported
		virtual std::unique_ptr<MultiplayerLobby> makeMultiplayerLobby(const String& key) { return {}; }
		[[deprecated]] std::unique_ptr<MultiplayerLobby> makeMultiplayerSession(const String& key) { return makeMultiplayerLobby(key); }
		virtual bool canSetLobbyPrivacy() { return false; }

		virtual void multiplayerInvitationCancel() { }

		// When the user joins a session, this function should be called back to let the game know what session they should join
		// If the join happens before this method is called, then wait for this method to be called, and then call the callback
		virtual void setJoinCallback(PlatformJoinCallback callback) {}
		virtual void setPreparingToJoinCallback(PlatformPreparingToJoinCallback callback) {}
		virtual void setJoinErrorCallback(PlatformJoinErrorCallback callback) {}

		virtual bool needsInviteExtraData() const { return false; }
		virtual String inviteExtraDataFile() const { return ""; }
		virtual void setInviteExtraData(const Bytes& data) {}

		virtual std::shared_ptr<NetworkService> createNetworkService(uint16_t port) { return {}; }

		
		///////////////////////
		// Player info/profile
		
		virtual bool canShowPlayerInfo() const { return false; }
		virtual void showPlayerInfo(String playerId) {}


		////////////////////////////
		// Profanity/child controls
		virtual bool canShowReportedUserContent() const { return true; }

		virtual bool needsStringFiltering() const { return false; }
		virtual void setProfanityCheckLanguage(const String& language) {}
		
		virtual void setProfanityCheckForbiddenWordsList(Vector<String> words) {}

		// Some platforms have different methods to check for profanity on names. If not, default to standard.
		virtual Future<String> performNameProfanityCheck(String name)
		{
			return performProfanityCheck(std::move(name));
		}

		// Returns a censored version of this string
		virtual Future<String> performProfanityCheck(String text)
		{
			Promise<String> promise;
			promise.setValue(std::move(text));
			return promise.getFuture();
		}


		/////////////////
		// Soft keyboard
		virtual bool hasKeyboard() const { return false; }
		virtual std::shared_ptr<InputKeyboard> getKeyboard() const { return {}; }


		//////////////////////
		// Misc platform data

		virtual bool customHandlesOnlineErrors() const { return false; }
		virtual void handleOnlineError() {}

		virtual I18NLanguage getSystemLanguage() const { return I18NLanguage("en-GB"); }
		virtual bool useSystemOverscan() const { return false; }
		virtual float getSystemOverscan() const { return 1.0f; }

		virtual bool canShowSubscriptionNeeded() const { return false; }
		virtual bool showSubscriptionNeeded() const { return true; }
		virtual bool canShowEULA() const { return true; }
		
		// Returns arbitrary platform-specific data
		virtual String getStringData(const String& key) { return ""; }

		virtual bool useSystemDisplaySize() { return false; }
		virtual Vector2i getSystemDisplaySize() { return Vector2i(0, 0); }
	};
}
