#include "httplib_http_request.h"

using namespace Halley;

HTTPLibHTTPRequest::HTTPLibHTTPRequest(HTTPMethod method, const String& url)
{
	
}

void HTTPLibHTTPRequest::setPostData(const String& contentType, const Bytes& data)
{
	
}

void HTTPLibHTTPRequest::setHeader(const String& headerName, const String& headerValue)
{
	
}

Future<std::unique_ptr<HTTPResponse>> HTTPLibHTTPRequest::send()
{
	return {};
}
