#pragma once

union SDL_Event;

namespace Halley
{
	enum class WindowType {
		None,
		Fullscreen,
		Window,
		BorderlessWindow
	};

	class VideoAPI
	{
	public:
		virtual ~VideoAPI() {}

		virtual void startRender() = 0;
		virtual void finishRender() = 0;
		virtual void flip() = 0;

		virtual void setVideo(WindowType windowType, Vector2i fullscreenSize, Vector2i windowedSize, Vector2f virtualSize = Vector2f(), int screen = 0) = 0;
		virtual Vector2i getWindowSize() const = 0;
		virtual Vector2f getVirtualSize() const = 0;
		virtual Vector2f getDisplaySize() const = 0;
		virtual Vector2f getOrigin() const = 0;
		virtual Vector2i getScreenSize(int n = 0) const = 0;
		virtual Rect4i getWindowRect() const = 0;
		virtual Rect4i getDisplayRect() const = 0;
		virtual float getBorder() const = 0;

		virtual float getScale() const = 0;
		virtual bool isFullscreen() const = 0;
		virtual void setFullscreen(bool isFullscreen) = 0;
		virtual void toggleFullscreen() = 0;

		virtual void setVirtualSize(Vector2f virtualSize) = 0;

	protected:
		friend class HalleyAPI;
		friend class SystemAPI;

		virtual void init() = 0;
		virtual void deInit() = 0;
		virtual void processEvent(SDL_Event& event) = 0;

		virtual void setWindowSize(Vector2i windowSize) = 0;
	};
}
