#include "httplib_web_api.h"
#include "httplib_http_request.h"

using namespace Halley;

void HTTPLibWebAPI::init()
{
}

void HTTPLibWebAPI::deInit()
{
}

std::unique_ptr<HTTPRequest> HTTPLibWebAPI::makeHTTPRequest(HTTPMethod method, const String& url)
{
	return std::make_unique<HTTPLibHTTPRequest>(method, url);
}
