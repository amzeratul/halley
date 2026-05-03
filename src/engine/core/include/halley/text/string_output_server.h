#pragma once

namespace Halley {
	class HTTPServer;
	class WebServerAPI;

	class StringOutputServer {
	public:
		StringOutputServer(WebServerAPI* webServerAPI);
		~StringOutputServer();

		bool start(const String& host, int port);
		bool stop();

	private:
		WebServerAPI* webServerAPI = nullptr;
		std::unique_ptr<HTTPServer> httpServer;

		void setupEndpoints();
	};
}
