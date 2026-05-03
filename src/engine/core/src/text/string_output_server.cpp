#include "halley/text/string_output_server.h"

#include <halley/api/web_api.h>

using namespace Halley;

StringOutputServer::StringOutputServer(WebServerAPI* webServerAPI)
	: webServerAPI(webServerAPI)
{
}

StringOutputServer::~StringOutputServer()
{
	stop();
}

bool StringOutputServer::start(const String& host, int port)
{
	if (webServerAPI && !httpServer) {
		httpServer = webServerAPI->makeHTTPServer();
		if (httpServer) {
			setupEndpoints();
			httpServer->listen(host, port);
			Logger::logInfo("String output server listening on http://" + host + ":" + port);
			return true;
		}
	}
	return false;
}

bool StringOutputServer::stop()
{
	if (httpServer) {
		httpServer = {};
		return true;
	}
	return false;
}

void StringOutputServer::setupEndpoints()
{
	httpServer->endpointGet("/test", [] (const HTTPServerRequest& request, HTTPServerResponse& response)
	{
		response.setContent("Hello world", "text/html");
	});
}
