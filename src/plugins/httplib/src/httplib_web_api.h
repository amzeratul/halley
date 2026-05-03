#pragma once
#include "halley/api/halley_api_internal.h"

namespace Halley {
	class HTTPLibWebAPI : public WebAPIInternal {
	public:
		void init() override;
		void deInit() override;

		std::unique_ptr<HTTPRequest> makeHTTPRequest(HTTPMethod method, const String& url) override;
	};

	class HTTPLibWebServerAPI : public WebServerAPIInternal {
	public:
		void init() override;
		void deInit() override;

		std::unique_ptr<HTTPServer> makeHTTPServer() override;
	};
}
