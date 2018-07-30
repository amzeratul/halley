#ifdef WINDOWS_STORE

#include "winrt_platform.h"
#include "winrt_http.h"
#include "xbl_manager.h"
using namespace Halley;


void WinRTPlatform::init()
{
	xbl = std::make_shared<XBLManager>();
}

void WinRTPlatform::deInit()
{
	xbl.reset();
}

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

#endif
