#pragma once
#include "halley/api/halley_api_internal.h"

namespace Halley {
	class DummyWebAPI : public WebAPIInternal {
	public:
		void init() override;
		void deInit() override;
		std::unique_ptr<HTTPRequest> makeHTTPRequest(HTTPMethod method, String url) override;
	};
}
