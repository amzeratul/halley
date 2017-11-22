#pragma once
#include "api/halley_api_internal.h"

namespace Halley {
	class DummyPlatformAPI : public PlatformAPIInternal {
	public:
		void init() override;
		void deInit() override;

		void update() override;

		std::unique_ptr<HTTPRequest> makeHTTPRequest(const String& method, const String& url) override;

		bool canProvideAuthToken() const override;
		Future<Bytes> getAuthToken() override;
	};
}
