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
		Error,
		ErrorAlreadyReported,
		Cancelled
	};

	struct AuthTokenResult {
		AuthTokenRetrievalResult result = AuthTokenRetrievalResult::Error;
		std::unique_ptr<AuthorisationToken> token;

		AuthTokenResult(AuthTokenRetrievalResult result)
			: result(result)
		{}
		AuthTokenResult(std::unique_ptr<AuthorisationToken> token)
			: result(AuthTokenRetrievalResult::OK)
			, token(std::move(token))
		{}
	};

	class PlatformAPI
	{
	public:
		virtual ~PlatformAPI() {}

		virtual void update() = 0;

		virtual std::unique_ptr<HTTPRequest> makeHTTPRequest(const String& method, const String& url) = 0;

		virtual bool canProvideAuthToken() const = 0;
		virtual Future<AuthTokenResult> getAuthToken() = 0;

		virtual bool canShowSubscriptionNeeded() const { return false; }

		// Returns true if the user has potentially accepted subscription
		virtual bool showSubscriptionNeeded() const { return false; }

		virtual bool canProvideCloudSave() const { return false; }
		virtual std::shared_ptr<ISaveData> getCloudSaveContainer(const String& containerName = "") { return {}; }
	};
}
