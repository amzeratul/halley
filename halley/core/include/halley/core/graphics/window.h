#pragma once
#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <boost/optional.hpp>
#include <halley/text/halleystring.h>

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
	
	class WindowDefinition
	{
	public:
		WindowDefinition(WindowType windowType, Vector2i size, String title)
			: windowType(windowType)
			, size(size)
			, title(title)
		{}

		WindowDefinition(WindowType windowType, boost::optional<Vector2i> position, Vector2i size, String title)
			: windowType(windowType)
			, position(position)
			, size(size)
			, title(title)
		{}

		WindowType getWindowType() const { return windowType; }
		boost::optional<Vector2i> getPosition() const { return position; }
		Vector2i getSize() const { return size; }
		String getTitle() const { return title; }

		WindowDefinition withPosition(boost::optional<Vector2i> newPos) const { return WindowDefinition(windowType, newPos, size, title); }
		WindowDefinition withSize(Vector2i newSize) const { return WindowDefinition(windowType, position, newSize, title); }

	private:
		WindowType windowType;
		boost::optional<Vector2i> position;
		Vector2i size;
		String title;
	};

	class Window
	{
	public:
		virtual ~Window() {}

		virtual void update(const WindowDefinition& definition) = 0;
		virtual void show() = 0;
		virtual void hide() = 0;
		virtual void setVsync(bool vsync) = 0;
		virtual void swap() = 0;
		virtual Rect4i getWindowRect() const = 0;
		virtual const WindowDefinition& getDefinition() const = 0;
	};
}
