#pragma once
#include <halley/maths/vector2d.h>
#include <halley/maths/rect.h>

namespace Halley
{
	class String;

	enum class WindowType {
		None,
		Fullscreen,
		Window,
		ResizableWindow,
		BorderlessWindow
	};
	
	class Window
	{
	public:
		Window(WindowType windowType, Vector2i size, String title, bool vsync = true, int screen = 0)
			: windowType(windowType)
			, size(size)
			, title(title)
			, vsync(vsync)
			, screen(screen)
		{}

		WindowType getWindowType() const { return windowType; }
		Vector2i getSize() const { return size; }
		String getTitle() const { return title; }
		bool isVSync() const { return vsync; }
		int getScreenNumber() const { return screen; }

		Window withSize(Vector2i newSize) const { return Window(windowType, newSize, title, vsync, screen); }

	private:
		WindowType windowType;
		Vector2i size;
		String title;
		bool vsync;
		int screen;
	};
}
