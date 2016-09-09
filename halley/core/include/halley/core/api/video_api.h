#pragma once
#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <halley/text/halleystring.h>

namespace Halley
{
	class Window;
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

		virtual void setWindow(Window&& windowDescriptor, bool vsync) = 0;
		virtual const Window& getWindow() const = 0;

		virtual Vector2i getScreenSize(int n) const = 0;
		virtual Rect4i getWindowRect() const = 0;
		virtual Rect4i getDisplayRect(int screen) const = 0;
		virtual Vector2i getCenteredWindow(Vector2i size, int screen) const = 0;

		virtual void resizeWindow(Rect4i windowSize) = 0;

		virtual std::unique_ptr<Texture> createTexture(const TextureDescriptor& descriptor) = 0;
		virtual std::unique_ptr<Shader> createShader(String name) = 0;
		virtual std::unique_ptr<TextureRenderTarget> createRenderTarget() = 0;
	};
}
