#pragma once

#include <memory>
#include <map>
#include <halley/concurrency/future.h>
#include <halley/utils/utils.h>

namespace Halley
{
	class ISaveData;
	class String;

	class HTTPResponse {
	public:
		virtual ~HTTPResponse() {}

		virtual int getResponseCode() const = 0;
		virtual const Bytes& getBody() const = 0;

		void setCancelled() { cancelled = true; }
		bool isCancelled() const { return cancelled; }

	private:
		bool cancelled = false;
	};

	class HTTPRequest {
	public:
		virtual ~HTTPRequest() {}
		virtual void setPostData(const String& contentType, const Bytes& data) = 0;
		virtual void setHeader(const String& headerName, const String& headerValue) = 0;
		virtual Future<std::unique_ptr<HTTPResponse>> send() = 0;
	};

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

		OnlineCapabilities() {}
		OnlineCapabilities(bool onlinePlay, bool ugc)
			: onlinePlay(onlinePlay)
			, ugc(ugc)
		{
		}
		
		void setAll(bool value)
		{
			onlinePlay = value;
			ugc = value;
		}
		
		bool includes(const OnlineCapabilities& other)
		{
			auto check = [](bool has, bool otherHas)
			{
				return otherHas ? has : true;
			};
			return check(onlinePlay, other.onlinePlay)
				&& check(ugc, other.ugc);
		}
	};

	struct AuthTokenResult {
		AuthTokenRetrievalResult result = AuthTokenRetrievalResult::Unknown;
		std::unique_ptr<AuthorisationToken> token;
		OnlineCapabilities capabilitiesSupported;

		AuthTokenResult(AuthTokenRetrievalResult result)
			: result(result)
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

	class MultiplayerSession {
	public:
		virtual ~MultiplayerSession() = default;

		virtual void update() = 0; // This is called once per frame by the game
		virtual void setParameter(const String& key, const String& data) = 0;
		virtual void setLobbyCapacity(int numPlayers, int maxPlayers) = 0;
	};

	// This is the join callback, see PlatformAPI's method for more details
	struct PlatformJoinCallbackParameters {
		// Fill these as necessary
		String param;
	};
	using PlatformJoinCallback = std::function<void(PlatformJoinCallbackParameters)>;

	class PlatformAPI
	{
	public:
		virtual ~PlatformAPI() {}

		virtual void update() = 0;

		virtual std::unique_ptr<HTTPRequest> makeHTTPRequest(const String& method, const String& url) = 0;

		virtual bool canProvideAuthToken() const = 0;
		virtual Future<AuthTokenResult> getAuthToken(const AuthTokenParameters& parameters) = 0;

		virtual bool canProvideCloudSave() const { return false; }
		virtual std::shared_ptr<ISaveData> getCloudSaveContainer(const String& containerName = "") { return {}; }

		virtual bool canShowSubscriptionNeeded() const { return false; }
		virtual bool showSubscriptionNeeded() const { return true; }

		virtual bool canShowPlayerName() const { return false; };
		virtual String getPlayerName() { return ""; }

		// Complete when currentProgress == maximumValue
		virtual String getId() { return ""; }
		virtual void setAchievementProgress(const String& achievementId, int currentProgress, int maximumValue) {}
		virtual bool isAchievementUnlocked(const String& achievementId, bool defaultValue) { return defaultValue; }

		// Return empty unique_ptr if not supported
		virtual std::unique_ptr<MultiplayerSession> makeMultiplayerSession() { return {}; }

		// When the user joins a session, this function should be called back to let the game know what session they should join
		// If the join happens before this method is called, then wait for this method to be called, and then call the callback
		virtual void setJoinCallback(PlatformJoinCallback callback) {}
	};
}
