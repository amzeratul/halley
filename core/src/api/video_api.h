#pragma once

namespace Halley
{
	enum class WindowType {
		None,
		Fullscreen,
		Window,
		BorderlessWindow
	};

	class Painter;
	class Texture;
	class TextureDescriptor;
	class TextureRenderTarget;
	class Shader;

	class VideoAPI
	{
	public:
		virtual ~VideoAPI() {}

		virtual void startRender() = 0;
		virtual void finishRender() = 0;
		virtual void flip() = 0;

		virtual void setVideo(WindowType windowType, Vector2i fullscreenSize, Vector2i windowedSize, Vector2f virtualSize = Vector2f(), bool vsync = true, int screen = 0) = 0;
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

		virtual std::unique_ptr<Texture> createTexture(TextureDescriptor& descriptor) = 0;
		virtual std::unique_ptr<Shader> createShader(String name) = 0;
		virtual std::unique_ptr<TextureRenderTarget> createRenderTarget() = 0;
	};
}
