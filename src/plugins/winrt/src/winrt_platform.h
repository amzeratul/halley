#pragma once
#include "halley/core/api/halley_api_internal.h"

namespace Halley
{
	class WinRTSystem;
	class XBLManager;

	class WinRTPlatform : public PlatformAPIInternal
	{
	public:
		explicit WinRTPlatform(WinRTSystem* system);

		void init() override;
		void deInit() override;

		void update() override;

		std::unique_ptr<HTTPRequest> makeHTTPRequest(const String& method, const String& url) override;

		bool canProvideAuthToken() const override;
		Future<std::unique_ptr<AuthorisationToken>> getAuthToken() override;

		void onGameStarted();

	private:
		std::shared_ptr<XBLManager> xbl;
		WinRTSystem* system;
	};
}
