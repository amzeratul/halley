#pragma once
#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <boost/optional.hpp>

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
		Window(WindowType windowType, Vector2i size, String title)
			: windowType(windowType)
			, size(size)
			, title(title)
		{}

		Window(WindowType windowType, boost::optional<Vector2i> position, Vector2i size, String title)
			: windowType(windowType)
			, position(position)
			, size(size)
			, title(title)
		{}

		WindowType getWindowType() const { return windowType; }
		boost::optional<Vector2i> getPosition() const { return position; }
		Vector2i getSize() const { return size; }
		String getTitle() const { return title; }

		Window withPosition(boost::optional<Vector2i> newPos) const { return Window(windowType, newPos, size, title); }
		Window withSize(Vector2i newSize) const { return Window(windowType, position, newSize, title); }

	private:
		WindowType windowType;
		boost::optional<Vector2i> position;
		Vector2i size;
		String title;
	};
}
