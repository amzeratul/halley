#pragma once
#include <halley/maths/rect.h>
#include <halley/core/input/input_keyboard.h>
#include <halley/core/resources/resources.h>
#include <halley/core/graphics/text/font.h>
#include <halley/support/logger.h>

namespace Halley
{
	class Painter;

	class ConsoleWindow : public ILoggerSink
	{
	public:
		explicit ConsoleWindow(Resources& resources);
		~ConsoleWindow();

		void update(InputKeyboard& keyboard);
		void draw(Painter& painter, Rect4f bounds) const;

		void printLn(const String& line);

		void log(LoggerLevel level, const String& msg) override;

	private:
		void submit();

		String input;
		std::vector<String> buffer;
		std::vector<String> history;
		std::shared_ptr<const Texture> texture;
		std::shared_ptr<Material> backgroundMaterial;
		std::shared_ptr<const Font> font;
	};
}
