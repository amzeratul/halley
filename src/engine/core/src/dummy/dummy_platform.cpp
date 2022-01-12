#include "dummy_platform.h"

using namespace Halley;

void DummyPlatformAPI::init() {}
void DummyPlatformAPI::deInit() {}

String DummyPlatformAPI::getId()
{
	return "dummy";
}

void DummyPlatformAPI::update() {}

std::unique_ptr<HTTPRequest> DummyPlatformAPI::makeHTTPRequest(const String& method, const String& url)
{
	return {};	
}

bool DummyPlatformAPI::canProvideAuthToken() const
{
	return false;
}

Future<AuthTokenResult> DummyPlatformAPI::getAuthToken(const AuthTokenParameters& parameters)
{
	return {};
}
