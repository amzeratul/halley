#pragma once
#include <halley/maths/rect.h>
#include <halley/core/input/input_device.h>
#include <halley/core/graphics/text/font.h>
#include <halley/support/logger.h>
#include <halley/ui/ui_factory.h>
#include <halley/ui/ui_widget.h>
#include <halley/ui/widgets/ui_debug_console.h>

namespace Halley
{
	class Painter;

	class ConsoleWindow : public UIWidget, public ILoggerSink
	{
	public:
		explicit ConsoleWindow(UIFactory& ui);
		~ConsoleWindow();

		void log(LoggerLevel level, const String& msg) override;

	protected:
		void update(Time t, bool moved) override;

	private:
		std::vector<std::pair<LoggerLevel, String>> buffer;
		std::shared_ptr<UIDebugConsole> console;

		UIDebugConsoleController controller;

		mutable std::mutex mutex;
	};
}
