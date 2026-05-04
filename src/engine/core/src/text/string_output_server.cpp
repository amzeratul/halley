#include "halley/text/string_output_server.h"

#include <halley/api/web_api.h>

#include "string_output_stream.h"

using namespace Halley;

ConfigNode StringOutputMetrics::toConfigNode() const
{
	ConfigNode result;
	if (rect) {
		result["rect"] = rect;
	}
	if (fontName) {
		result["fontName"] = fontName;
	}
	if (fontSize) {
		result["fontSize"] = fontSize;
	}
	return result;
}

StringOutputServer::StringOutputServer(const I18N& i18n)
	: i18n(i18n)
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
			curState = std::make_unique<StringOutputState>();
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
	curState->startFrame();
}

void StringOutputServer::endFrame()
{
	curState->endFrame();
}

void StringOutputServer::startUI(const UIRoot& uiRoot)
{
	// TODO
}

void StringOutputServer::endUI(const UIRoot& uiRoot)
{
	// TODO
}

void StringOutputServer::reportString(const String& id, StringOutputType type, const LocalisedString& string, const StringOutputMetrics& metrics)
{
	curState->reportString(id, type, string, metrics, i18n);
}

void StringOutputServer::setupEndpoints()
{
	httpServer->endpointGet("/test", [] (const HTTPServerRequest& request, HTTPServerResponse& response)
	{
		response.setContent("Hello world", "text/html");
	});

	httpServer->endpointGet("/strings", [this](const HTTPServerRequest& request, HTTPServerResponse& response)
	{
		UniqueLock lock(curState->mutex);
		auto stream = std::make_shared<StringOutputStream>(*curState);
		lock.unlock();

		response.setChunkedContentProvider("text/event-stream", [stream = std::move(stream)](HTTPServerDataSink& sink) mutable -> bool
		{
			stream->waitAndUpdateState();
			stream->outputDelta(sink);
			return true;
		});
	});
}
