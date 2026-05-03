#include "halley/text/string_output_server.h"

#include <halley/api/web_api.h>

using namespace Halley;

StringOutputServer::StringOutputServer()
{
}

StringOutputServer::~StringOutputServer()
{
	stopServer();
}

bool StringOutputServer::startServer(WebServerAPI* webServerAPI, const String& host, int port)
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

bool StringOutputServer::stopServer()
{
	if (httpServer) {
		httpServer = {};
		return true;
	}
	return false;
}

void StringOutputServer::startFrame()
{
	// TODO
}

void StringOutputServer::endFrame()
{
	// TODO
}

void StringOutputServer::startUI(const UIRoot& uiRoot)
{
	// TODO
}

void StringOutputServer::endUI(const UIRoot& uiRoot)
{
	// TODO
}

void StringOutputServer::reportString(StringOutputType type, const String& id, const LocalisedString& string, const StringOutputMetrics& metrics)
{
	//Logger::logDev(id + ": " + string.getString(), true);
}

void StringOutputServer::setupEndpoints()
{
	httpServer->endpointGet("/test", [] (const HTTPServerRequest& request, HTTPServerResponse& response)
	{
		response.setContent("Hello world", "text/html");
	});

	httpServer->endpointGet("/strings", [] (const HTTPServerRequest& request, HTTPServerResponse& response)
	{
		// TODO
		response.setChunkedContentProvider("text/event-stream", [=] (HTTPServerDataSink& sink) -> bool
		{
			// TODO
			//sink.write();
			return true;
		});
	});
}
