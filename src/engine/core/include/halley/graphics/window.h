#pragma once
#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <halley/text/halleystring.h>
#include <halley/file/path.h>
#include <halley/data_structures/maybe.h>

#include "halley/maths/colour.h"

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

	enum class WindowState
	{
		Normal,
		Minimized,
		Maximized
	};

	template <>
	struct EnumNames<WindowType> {
		constexpr std::array<const char*, 5> operator()() const {
			return {{
				"none",
				"fullscreen",
				"window",
				"resizeableWindow",
				"borderlessWindow",
			}};
		}
	};

	struct WindowGLVersion {
		int versionMajor = 3;
		int versionMinor = 3;
	};
	
	class WindowDefinition
	{
	public:
		WindowDefinition() = default;

		WindowDefinition(WindowType windowType, Vector2i size, String title, bool showOnCreation = true, int screen = 0, std::optional<WindowGLVersion> glVersion = WindowGLVersion())
			: windowType(windowType)
			, size(size)
			, title(std::move(title))
			, showOnCreation(showOnCreation)
			, screen(screen)
			, glVersion(glVersion)
		{}

		WindowDefinition(WindowType windowType, std::optional<Vector2i> position, Vector2i size, String title, bool showOnCreation = true, int screen = 0, std::optional<WindowGLVersion> glVersion = WindowGLVersion())
			: windowType(windowType)
			, position(position)
			, size(size)
			, title(std::move(title))
			, showOnCreation(showOnCreation)
			, screen(screen)
			, glVersion(glVersion)
		{}

		WindowType getWindowType() const { return windowType; }
		WindowState getWindowState() const { return windowState; }
		std::optional<Vector2i> getPosition() const { return position; }
		Vector2i getSize() const { return size; }
		String getTitle() const { return title; }
		std::optional<Path> getIcon() const { return icon; }
		bool isShowOnCreation() const { return showOnCreation; }
		int getScreen() const { return screen; }
		bool isFocusLost() const { return focusLost; }
		std::optional<WindowGLVersion> getWindowGLVersion() const { return *glVersion; }

		WindowDefinition withPosition(std::optional<Vector2i> newPos) const
		{
			auto w = *this;
			w.position = newPos;
			return w;
		}
		
		WindowDefinition withSize(Vector2i newSize) const
		{
			auto w = *this;
			w.size = newSize;
			return w;
		}

		WindowDefinition withState(WindowState newState) const
		{
			auto w = *this;
			w.windowState = newState;
			return w;
		}

		WindowDefinition withIcon(const Path& iconPath) const
		{
			auto w = *this;
			w.icon = iconPath;
			return w;
		}

		WindowDefinition withFocus(bool hasFocus) const
		{
			auto w = *this;
			w.focusLost = !hasFocus;
			return w;
		}

	private:
		WindowType windowType = WindowType::Fullscreen;
		WindowState windowState = WindowState::Normal;
		std::optional<Vector2i> position;
		Vector2i size;
		String title;
		std::optional<Path> icon;
		bool showOnCreation = true;
		int screen = 0;
		bool focusLost = false;
		std::optional<WindowGLVersion> glVersion;
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

		virtual void* getHandle() const { return nullptr; }
		virtual void* getNativeHandle() const { return nullptr; }
		virtual String getNativeHandleType() const { return ""; }

		virtual void setTitleColour(Colour4f bgCol, Colour4f textCol) {}
	};
}
