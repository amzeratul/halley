#pragma once

#include "halley/core/api/halley_api_internal.h"
#include <SDL.h>
#include <memory>

namespace Halley
{
	class SDLWindow;
	class Window;

	class SystemSDL final : public SystemAPIInternal
	{
	public:
		explicit SystemSDL(std::optional<String> saveCryptKey);

	protected:
		void init() override;
		void deInit() override;

		void onResume() override;
		void onSuspend() override;

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

		std::shared_ptr<ISaveData> getStorageContainer(SaveDataType type, const String& containerName) override;

		void setEnvironment(Environment* env) override;

		bool canExit() override;

		std::shared_ptr<IClipboard> getClipboard() const override;

		void setThreadName(const String& name) override;

		void registerGlobalHotkey(KeyCode key, KeyMods keyMods, std::function<void()> callback) override;
	
	private:
		void processVideoEvent(VideoAPI* video, const SDL_Event& event);
		void processSystemEvent(const SDL_Event& event);

		void printDebugInfo() const;

		void initVideo() const;
		void deInitVideo();
		
		Vector<std::shared_ptr<SDLWindow>> windows;
		mutable bool videoInit = false;
		std::map<SaveDataType, Path> saveDir;
		std::shared_ptr<IClipboard> clipboard;
		std::optional<String> saveCryptKey;
		Vector<std::function<void()>> globalHotkeyCallbacks;
	};
}
