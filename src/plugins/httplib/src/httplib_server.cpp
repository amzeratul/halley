#include "httplib_server.h"

using namespace Halley;

HTTPLibServerDataSink::HTTPLibServerDataSink(httplib::DataSink& dataSink)
	: dataSink(dataSink)
{
}

void HTTPLibServerDataSink::write(gsl::span<const std::byte> bytes)
{
	dataSink.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

void HTTPLibServerDataSink::done()
{
	dataSink.done();
}

HTTPLibServerRequest::HTTPLibServerRequest(const httplib::Request& request)
	: request(request)
{
}

HTTPLibServerResponse::HTTPLibServerResponse(httplib::Response& response)
	: response(response)
{
}

void HTTPLibServerResponse::setHeader(const String& header, const String& data)
{
	response.set_header(header, data);
}

void HTTPLibServerResponse::setContent(gsl::span<const std::byte> data, const String& contentType)
{
	response.set_content(reinterpret_cast<const char*>(data.data()), data.size(), contentType.cppStr());
}

void HTTPLibServerResponse::setChunkedContentProvider(const String& contentType, Callback callback)
{
	response.set_chunked_content_provider(contentType.cppStr(), [callback = std::move(callback)](size_t offset, httplib::DataSink& dataSink) -> bool
	{
		auto d = HTTPLibServerDataSink(dataSink);
		return callback(d);
	});
}

HTTPLibWebServer::~HTTPLibWebServer()
{
	server.stop();
	thread.join();
}

void HTTPLibWebServer::endpointGet(const String& endpoint, Handler handler)
{
	server.Get(endpoint, [handler = std::move(handler)](const httplib::Request& request, httplib::Response& response)
	{
		auto req = HTTPLibServerRequest(request);
		auto res = HTTPLibServerResponse(response);
		handler(req, res);
	});
}

void HTTPLibWebServer::listen(const String& host, int port)
{
	thread = std::thread([this, host, port] () {
		server.listen(host.cppStr(), port);
	});
}
