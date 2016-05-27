struct SDL_Window;

#include "halley/core/api/halley_api_internal.h"

namespace Halley {
	class VideoOpenGL final : public VideoAPIInternal
	{
	public:
		VideoOpenGL();

		void startRender() override;
		void finishRender() override;
		void flip() override;

		void setVideo(WindowType windowType, Vector2i fullscreenSize, Vector2i windowedSize, Vector2f virtualSize = Vector2f(), bool vsync = true, int screen = 0) override;
		Vector2i getWindowSize() const override { return windowSize; }
		Vector2f getVirtualSize() const override { return virtualSize; }
		Vector2f getDisplaySize() const override { return p2 - p1; }
		Vector2f getOrigin() const override { return p1; }
		Vector2i getScreenSize(int n = 0) const override;
		Rect4i getWindowRect() const override;
		Rect4i getDisplayRect() const override;
		float getBorder() const override { return border; }

		float getScale() const override { return scale; }
		bool isFullscreen() const override { return windowType == WindowType::Fullscreen; }
		void setFullscreen(bool isFullscreen) override;
		void toggleFullscreen() override;

		void setVirtualSize(Vector2f virtualSize) override;

		std::function<void(int, void*)> getUniformBinding(UniformType type, int n) override;
		std::unique_ptr<Painter> makePainter() override;
		std::unique_ptr<Texture> createTexture(const TextureDescriptor& descriptor) override;
		std::unique_ptr<Shader> createShader(String name) override;
		std::unique_ptr<TextureRenderTarget> createRenderTarget() override;

	protected:
		void init() override;
		void deInit() override;
		void processEvent(SDL_Event& event) override;

	private:
		void updateWindowDimensions();
		void setWindowSize(Vector2i windowSize);
		void setUpDebugCallback();
		void setUpEnumMap();
		void onGLDebugMessage(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, String message) const;

		std::map<unsigned int, String> glEnumMap;
		mutable std::vector<std::function<void()>> messagesPending;
		mutable std::mutex messagesMutex;

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
