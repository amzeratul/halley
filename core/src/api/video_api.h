#pragma once

union SDL_Event;
struct SDL_Window;

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
		VideoAPI();

		void flip();

		void setVideo(WindowType windowType, Vector2i fullscreenSize, Vector2i windowedSize, Vector2f virtualSize = Vector2f(), int screen = 0);
		Vector2i getWindowSize() const { return windowSize; }
		Vector2f getVirtualSize() const { return virtualSize; }
		Vector2f getDisplaySize() const { return p2 - p1; }
		Vector2f getOrigin() const { return p1; }
		Vector2i getScreenSize(int n = 0) const;
		Rect4i getWindowRect() const;
		Rect4i getDisplayRect() const;
		float getBorder() const { return border; }

		float getScale() const { return scale; }
		bool isFullscreen() const { return windowType == WindowType::Fullscreen; }
		void setFullscreen(bool isFullscreen);
		void toggleFullscreen();

		void setVirtualSize(Vector2f virtualSize);

	private:
		friend class HalleyAPI;
		friend class CoreAPI;

		void init();
		void deInit();
		void processEvent(SDL_Event& event);

		void setWindowSize(Vector2i windowSize);
		void updateWindowDimensions();

		void* context;
		Vector2i windowSize;
		Vector2i windowedSize;
		Vector2i fullscreenSize;
		Vector2f virtualSize;
		Vector2f p1, p2;
		WindowType windowType;
		bool initialized;
		bool running;
		float scale;
		float border;
		SDL_Window* window;
	};
}
