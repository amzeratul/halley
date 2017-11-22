#include "dummy_platform.h"

using namespace Halley;

void DummyPlatformAPI::init() {}
void DummyPlatformAPI::deInit() {}
void DummyPlatformAPI::update() {}

std::unique_ptr<HTTPRequest> DummyPlatformAPI::makeHTTPRequest(const String& method, const String& url)
{
	return {};	
}

bool DummyPlatformAPI::canProvideAuthToken() const
{
	return false;
}

Future<Bytes> DummyPlatformAPI::getAuthToken()
{
	return {};
}
