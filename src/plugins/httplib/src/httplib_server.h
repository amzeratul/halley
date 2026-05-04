#pragma once
#include <halley/api/web_api.h>
#include "../contrib/cpp-httplib/httplib.h"

namespace Halley {
	class HTTPLibServerDataSink : public HTTPServerDataSink {
	public:
		HTTPLibServerDataSink(httplib::DataSink& dataSink);

		void write(gsl::span<const std::byte> bytes) override;
		void done() override;

	private:
		httplib::DataSink& dataSink;
	};

	class HTTPLibServerRequest : public HTTPServerRequest {
	public:
		HTTPLibServerRequest(const httplib::Request& request);

	private:
		const httplib::Request& request;
	};

	class HTTPLibServerResponse : public HTTPServerResponse {
	public:
		HTTPLibServerResponse(httplib::Response& response);

		void setHeader(const String& header, const String& data) override;
		void setContent(gsl::span<const std::byte> data, const String& contentType) override;
		void setChunkedContentProvider(const String& contentType, Callback callback) override;

	private:
		httplib::Response& response;
	};

	class HTTPLibWebServer : public HTTPServer {
	public:
		~HTTPLibWebServer() override;

		void endpointGet(const String& endpoint, Handler handler) override;
		void listen(const String& host, int port) override;

	private:
		httplib::Server server;
		std::thread thread;
	};
}