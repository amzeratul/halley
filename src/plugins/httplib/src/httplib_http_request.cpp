#include "httplib_http_request.h"
#include "halley/concurrency/concurrent.h"

using namespace Halley;

#ifdef DELETE
#undef DELETE
#endif

namespace {
	std::pair<String, String> parseURL(const String& url)
	{
		const auto protocolDivider = url.find("://");

		const auto pos = url.find('/', protocolDivider == std::string::npos ? 0 : protocolDivider + 3);
		if (pos == std::string::npos) {
			return { url, "/" };
		}

		return { url.substr(0, pos), url.substr(pos) };
	}
}

HTTPLibHTTPRequest::HTTPLibHTTPRequest(HTTPMethod method, const String& url)
	: HTTPLibHTTPRequest(method, parseURL(url))
{
}

HTTPLibHTTPRequest::HTTPLibHTTPRequest(HTTPMethod method, std::pair<String, String> hostAndPath)
	: method(method)
	, host(hostAndPath.first)
	, path(hostAndPath.second)
{
	progress = [](uint64_t cur, uint64_t total)
	{
		return true;
	};
}

void HTTPLibHTTPRequest::setPostData(const String& contentType, const Bytes& data)
{
	this->contentType = contentType;
	postData = data;
}

void HTTPLibHTTPRequest::setHeader(const String& headerName, const String& headerValue)
{
	headers.emplace(headerName.cppStr(), headerValue.cppStr());
}

void HTTPLibHTTPRequest::setProgressCallback(std::function<bool(uint64_t, uint64_t)> callback)
{
	progress = callback;
}

httplib::Result HTTPLibHTTPRequest::run()
{
	using namespace std::chrono_literals;
	httplib::Client client(host.cppStr());
	client.set_write_timeout(2s);
	client.set_read_timeout(2s);

	if (method == HTTPMethod::GET) {
		return client.Get(path.cppStr(), headers, progress);
	} else if (method == HTTPMethod::POST) {
		return client.Post(path.cppStr(), headers, reinterpret_cast<const char*>(postData.data()), postData.size(), contentType.cppStr());
	} else if (method == HTTPMethod::PUT) {
		return client.Put(path.cppStr(), headers, reinterpret_cast<const char*>(postData.data()), postData.size(), contentType.cppStr());
	} else if (method == HTTPMethod::DELETE) {
		return client.Delete(path.cppStr(), headers);
	} else if (method == HTTPMethod::PATCH) {
		return client.Patch(path.cppStr(), headers, reinterpret_cast<const char*>(postData.data()), postData.size(), contentType.cppStr());
	} else {
		throw Exception("Unknwown HTTP method: " + toString(method), HalleyExceptions::Web);
	}
}

Future<std::unique_ptr<HTTPResponse>> HTTPLibHTTPRequest::send()
{
	auto& self = *this;
	return Concurrent::execute(Executors::getImmediate(), [&self] () -> HTTPLibHTTPRequest
	{
		return HTTPLibHTTPRequest(std::move(self));
	}).then(Executors::getCPU(), [] (HTTPLibHTTPRequest request) mutable -> std::unique_ptr<HTTPResponse>
	{
		return std::make_unique<HTTPLibHTTPResponse>(request.run());
	});
}

HTTPLibHTTPResponse::HTTPLibHTTPResponse(httplib::Result result)
{
	if (result) {
		responseCode = result->status;
		body = Bytes(std::begin(result->body), std::end(result->body));
		if (responseCode == 301) {
			if (result->has_header("Location")) {
				location = result->get_header_value("Location");
			}
		}
	} else {
		responseCode = 0;
	}
}

int HTTPLibHTTPResponse::getResponseCode() const
{
	return responseCode;
}

const Bytes& HTTPLibHTTPResponse::getBody() const
{
	return body;
}

Bytes HTTPLibHTTPResponse::moveBody()
{
	return std::move(body);
}

String HTTPLibHTTPResponse::getRedirectLocation()
{
	return location;
}
