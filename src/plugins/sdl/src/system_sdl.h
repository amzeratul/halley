#pragma once

#include "halley/core/api/halley_api_internal.h"
#include <SDL.h>

namespace Halley
{
	class SDLWindow;

	class SystemSDL final : public SystemAPIInternal
	{
	protected:
		void init() override;
		void deInit() override;

		Path getAssetsPath(const Path& gamePath) const override;
		Path getUnpackedAssetsPath(const Path& gamePath) const override;

		bool generateEvents(VideoAPI* video, InputAPI* input) override;

		std::unique_ptr<ResourceDataReader> getDataReader(String path, int64_t start, int64_t end) override;

		std::shared_ptr<Window> createWindow(const WindowDefinition& window) override;
		void destroyWindow(std::shared_ptr<Window> window) override;

		Vector2i getScreenSize(int n) const override;
		Rect4i getDisplayRect(int screen) const override;
		Vector2i getCenteredWindow(Vector2i size, int screen) const;
		std::unique_ptr<GLContext> createGLContext() override;

		void showCursor(bool show) override;

		Bytes getSaveData(SaveDataType type, const String& path) override;
		void setSaveData(SaveDataType type, const String& path, const Bytes& data) override;
		std::vector<String> enumerateSaveData(SaveDataType type, const String& root) override;

		void setEnvironment(Environment* env) override;

		bool canExit() override;

	private:
		void processVideoEvent(VideoAPI* video, const SDL_Event& event);

		void printDebugInfo() const;

		void initVideo() const;
		void deInitVideo();

	private:
		std::vector<std::shared_ptr<SDLWindow>> windows;
		mutable bool videoInit = false;
		Path saveDir;
		Path cacheDir;
	};
}
