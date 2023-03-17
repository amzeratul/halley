#include "dummy_web.h"

using namespace Halley;

void DummyWebAPI::init()
{
}

void DummyWebAPI::deInit()
{
}

std::unique_ptr<HTTPRequest> DummyWebAPI::makeHTTPRequest(HTTPMethod method, const String& url)
{
	return {};	
}
