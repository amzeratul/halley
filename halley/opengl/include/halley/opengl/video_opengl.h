#include <map>
#include <mutex>
#include <halley/data_structures/flat_map.h>
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
		
		void setWindow(Window&& window, bool vsync) override;
		const Window& getWindow() const override;

		Vector2i getScreenSize(int n) const override;
		Rect4i getWindowRect() const override;
		Rect4i getDisplayRect(int screen) const override;
		Vector2i getCenteredWindow(Vector2i size, int screen) const override;

		std::function<void(int, void*)> getUniformBinding(UniformType type, int n) override;
		std::unique_ptr<Painter> makePainter() override;
		std::unique_ptr<Texture> createTexture(const TextureDescriptor& descriptor) override;
		std::unique_ptr<Shader> createShader(String name) override;
		std::unique_ptr<TextureRenderTarget> createRenderTarget() override;

	protected:
		void init() override;
		void deInit() override;

		void onSuspend() override;
		void onResume() override;

		void processEvent(SDL_Event& event) override;

	private:
		void printDebugInfo() const;
		void createWindow(const Window& window);
		void updateWindow(const Window& window);
		void initOpenGL(bool vsync);
		void initGLBindings();
		void clearScreen();

		void setupDebugCallback();
		void setUpEnumMap();
		void onGLDebugMessage(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, String message) const;

		FlatMap<unsigned int, String> glEnumMap;
		mutable Vector<std::function<void()>> messagesPending;
		mutable std::mutex messagesMutex;

		void* context = nullptr;
		bool initialized = false;
		bool running = false;
		SDL_Window* sdlWindow = nullptr;
		std::unique_ptr<Window> curWindow;
	};
}
