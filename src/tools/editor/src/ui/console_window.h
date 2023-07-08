#pragma once
#include <halley/support/logger.h>
#include <halley/ui/ui_widget.h>
#include <halley/ui/widgets/ui_debug_console.h>

namespace Halley
{
	class StatusBar;
	class Painter;

	class ConsoleWindow : public UIWidget, public ILoggerSink
	{
	public:
		explicit ConsoleWindow(UIFactory& ui, const HalleyAPI& api);
		~ConsoleWindow();

		void log(LoggerLevel level, const std::string_view msg) override;
		void setStatusBar(std::shared_ptr<StatusBar> statusBar);

		static Colour4f getColour(const UIColourScheme& colourScheme, LoggerLevel level);

	protected:
		void update(Time t, bool moved) override;

	private:
		UIFactory& factory;
		
		Vector<std::pair<LoggerLevel, String>> buffer;
		std::shared_ptr<UIDebugConsole> console;

		std::shared_ptr<UIDebugConsoleController> controller;

		std::shared_ptr<StatusBar> statusBar;

		mutable std::mutex mutex;
	};
}
