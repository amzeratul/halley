#pragma once
#include <halley/api/web_api.h>

namespace Halley {
	class HTTPLibHTTPRequest : public HTTPRequest {
	public:
		HTTPLibHTTPRequest(HTTPMethod method, const String& url);
		void setPostData(const String& contentType, const Bytes& data) override;
		void setHeader(const String& headerName, const String& headerValue) override;
		Future<std::unique_ptr<HTTPResponse>> send() override;
	};
}
