#pragma once

#include <map>
#include <mutex>
#include <halley/data_structures/flat_map.h>
#include "halley/api/halley_api_internal.h"
#include "halley/graphics/window.h"
#include "loader_thread_opengl.h"

namespace Halley {
	class SystemAPI;

	class VideoOpenGL final : public VideoAPIInternal
	{
	public:
		VideoOpenGL(SystemAPI& system);

		void startRender() override;
		void finishRender() override;
		
		void setWindow(WindowDefinition&& window) override;
		Window& getWindow() const override;
		bool hasWindow() const override;
		void setVsync(bool vsync) override;

		std::unique_ptr<Painter> makePainter(Resources& resources) override;
		std::unique_ptr<Texture> createTexture(Vector2i size) override;
		std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) override;
		std::unique_ptr<TextureRenderTarget> createTextureRenderTarget() override;
		std::unique_ptr<ScreenRenderTarget> createScreenRenderTarget() override;
		std::unique_ptr<MaterialConstantBuffer> createConstantBuffer() override;
		std::unique_ptr<MaterialShaderStorageBuffer> createShaderStorageBuffer() override;

		String getShaderLanguage() override;
		bool isColumnMajor() const override;

		bool isLoaderThread() const;

		static void initGLBindings();

	protected:
		void init() override;
		void deInit() override;

		void onSuspend() override;
		void onResume() override;

	private:
		void initOpenGL();
		void clearScreen();
		void startLoaderThread();
		void flip();

		void setupDebugCallback();
		void setUpEnumMap();
		void onGLDebugMessage(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, String message) const;

		SystemAPI& system;

		FlatMap<unsigned int, String> glEnumMap;
		mutable Vector<std::function<void()>> messagesPending;
		mutable std::mutex messagesMutex;

		std::unique_ptr<GLContext> context;
		bool initialized = false;

		std::unique_ptr<LoaderThreadOpenGL> loaderThread;
				
		std::shared_ptr<Window> window;
		bool useVsync = false;
	};
}
