#pragma once
#include <halley/maths/rect.h>
#include <halley/core/input/input_keyboard.h>

namespace Halley
{
	class Painter;

	class ConsoleWindow
	{
	public:
		explicit ConsoleWindow(Resources& resources);
		void update(InputKeyboard& keyboard);
		void draw(Painter& painter, Rect4f bounds) const;

	private:
		void submit();

		String input;
		std::vector<String> buffer;
		std::vector<String> history;
		std::shared_ptr<Material> backgroundMaterial;
		std::shared_ptr<Font> font;
	};
}
