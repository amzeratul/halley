#pragma once
#include <halley/api/web_api.h>
#include "../contrib/cpp-httplib/httplib.h"

namespace Halley {

	class HTTPLibHTTPRequest : public HTTPRequest {
	public:
		HTTPLibHTTPRequest(HTTPLibHTTPRequest&& other) = default;
		HTTPLibHTTPRequest(HTTPMethod method, const String& url);
		HTTPLibHTTPRequest(HTTPMethod method, std::pair<String, String> hostAndPath);

		HTTPLibHTTPRequest& operator=(HTTPLibHTTPRequest&& other) = default;

		void setPostData(const String& contentType, const Bytes& data) override;
		void setHeader(const String& headerName, const String& headerValue) override;
		Future<std::unique_ptr<HTTPResponse>> send() override;

	private:
		httplib::Headers headers;

		HTTPMethod method;
		String host;
		String path;
		String contentType;
		Bytes postData;

		httplib::Result run();
	};

	
	class HTTPLibHTTPResponse : public HTTPResponse {
	public:
		HTTPLibHTTPResponse(httplib::Result result);

		int getResponseCode() const override;
		const Bytes& getBody() const override;

	private:
		int responseCode;
		Bytes body;
	};
}
