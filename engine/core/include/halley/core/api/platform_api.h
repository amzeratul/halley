#pragma once

namespace Halley
{
	class HTTPResponse {
	public:
		virtual ~HTTPResponse() {}

		virtual const String& getBody() const = 0;
	};

	class HTTPRequest {
	public:
		virtual ~HTTPRequest() {}
		virtual Future<std::unique_ptr<HTTPResponse>> send() = 0;
	};

	class PlatformAPI
	{
	public:
		virtual ~PlatformAPI() {}

		virtual void update() = 0;

		virtual std::unique_ptr<HTTPRequest> makeHTTPRequest(const String& method, const String& url) = 0;

		virtual bool canProvideAuthToken() const = 0;
		virtual Future<Bytes> getAuthToken() = 0;
	};
}
