#pragma once
#include <halley/maths/rect.h>
#include <halley/core/input/input_device.h>
#include <halley/core/graphics/text/font.h>
#include <halley/support/logger.h>

namespace Halley
{
	class Painter;

	class ConsoleWindow : public UIWidget, public ILoggerSink
	{
	public:
		explicit ConsoleWindow(UIFactory& ui);
		~ConsoleWindow();

		void draw(UIPainter& painter) const override;

		void printLn(const String& line);
		void log(LoggerLevel level, const String& msg) override;

	private:
		void submit();

		String input;
		Sprite background;
		std::vector<String> buffer;
		std::vector<String> history;
		std::shared_ptr<const Font> font;

		mutable std::mutex mutex;
	};
}
