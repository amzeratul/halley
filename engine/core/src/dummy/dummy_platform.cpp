#include "dummy_platform.h"

using namespace Halley;

void DummyPlatformAPI::init() {}
void DummyPlatformAPI::deInit() {}

std::unique_ptr<HTTPRequest> DummyPlatformAPI::makeHTTPRequest(const String& method, const String& url)
{
	return {};	
}
