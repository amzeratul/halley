#pragma once
#include "halley/concurrency/alive_flag.h"
#include "halley/data_structures/config_node.h"
#include "halley/maths/rect.h"

namespace Halley {
	class I18N;
	class UIRoot;
	class LocalisedString;
	class HTTPServer;
	class WebServerAPI;
	class StringOutputState;
	class StringOutputStream;

	enum class StringOutputType {
		Generic,
		Dialogue,
		Description,
		UI,
	};

	template <>
	struct EnumNames<StringOutputType> {
		constexpr auto operator()() const {
			return std::to_array({
				"generic",
				"dialogue",
				"description",
				"ui"
			});
		}
	};

	struct StringOutputMetrics {
		std::optional<Rect4f> rect;
		std::optional<String> fontName;
		std::optional<float> fontSize;

		bool operator==(const StringOutputMetrics& other) const = default;
		bool operator!=(const StringOutputMetrics& other) const = default;

		ConfigNode toConfigNode() const;
	};

	class StringOutputServer {
	public:
		StringOutputServer(const I18N& i18n);
		~StringOutputServer();

		bool startServer(WebServerAPI* webServerAPI, const String& host, int port);
		bool stopServer();

		void startFrame();
		void endFrame();
		void startUI(const UIRoot& uiRoot);
		void endUI(const UIRoot& uiRoot);

		void reportString(const String& id, StringOutputType type, const LocalisedString& string, const StringOutputMetrics& metrics);

	private:
		const I18N& i18n;
		std::unique_ptr<HTTPServer> httpServer;
		std::unique_ptr<StringOutputState> curState;

		void setupEndpoints();
	};
}
