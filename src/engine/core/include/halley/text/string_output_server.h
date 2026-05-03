#pragma once
#include "halley/maths/rect.h"

namespace Halley {
	class UIRoot;
	class LocalisedString;
	class HTTPServer;
	class WebServerAPI;

	enum class StringOutputType {
		Generic,
		Dialogue,
		Description,
		UI,
	};

	struct StringOutputMetrics {
		std::optional<Rect4f> rect;
		std::optional<String> fontName;
		std::optional<float> fontSize;
	};

	class StringOutputServer {
	public:
		StringOutputServer();
		~StringOutputServer();

		bool startServer(WebServerAPI* webServerAPI, const String& host, int port);
		bool stopServer();

		void startFrame();
		void endFrame();
		void startUI(const UIRoot& uiRoot);
		void endUI(const UIRoot& uiRoot);

		void reportString(StringOutputType type, const String& id, const LocalisedString& string, const StringOutputMetrics& metrics);

	private:
		std::unique_ptr<HTTPServer> httpServer;

		void setupEndpoints();
	};
}
