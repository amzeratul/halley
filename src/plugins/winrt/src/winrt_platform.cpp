#include "winrt_platform.h"
#include "winrt_http.h"
using namespace Halley;


void WinRTPlatform::init() {}

void WinRTPlatform::deInit() {}

void WinRTPlatform::update() {}

std::unique_ptr<HTTPRequest> WinRTPlatform::makeHTTPRequest(const String& method, const String& url)
{
	return std::make_unique<WinRTHTTPRequest>(method, url);
}

bool WinRTPlatform::canProvideAuthToken() const
{
	return false;
}

Future<std::unique_ptr<AuthorisationToken>> WinRTPlatform::getAuthToken()
{
	Promise<std::unique_ptr<AuthorisationToken>> promise;
	promise.setValue({});
	return promise.getFuture();
}
