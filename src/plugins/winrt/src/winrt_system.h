#pragma once
#include <halley/support/logger.h>
#include "halley/core/api/halley_api_internal.h"
#include "winrt/Windows.Storage.h"

namespace Halley {
	class WinRTPlatform;

	class WinRTSystem : public SystemAPIInternal, public ILoggerSink {
	public:
		void init() override;
		void deInit() override;

		Path getAssetsPath(const Path& gamePath) const override;
		Path getUnpackedAssetsPath(const Path& gamePath) const override;
		std::unique_ptr<ResourceDataReader> getDataReader(String path, int64_t start, int64_t end) override;

		std::unique_ptr<GLContext> createGLContext() override;

		std::shared_ptr<Window> createWindow(const WindowDefinition& window) override;
		void destroyWindow(std::shared_ptr<Window> window) override;
		Vector2i getScreenSize(int n) const override;
		Rect4i getDisplayRect(int screen) const override;

		void showCursor(bool show) override;
		bool hasBeenDisconnectedFromTheInternet() override;

		std::shared_ptr<ISaveData> getStorageContainer(SaveDataType type, const String& containerName) override;

		bool generateEvents(VideoAPI* video, InputAPI* input) override;

		void runGame(std::function<void()> runnable) override;
		void log(LoggerLevel level, const String& msg) override;

		void setPlatform(WinRTPlatform* winrtPlatform);
		WinRTPlatform* getPlatform();

		void setOnSuspendCallback(SystemOnSuspendCallback callback) override { onSuspendCallback = callback; }
		void callOnSuspendCallback();

		void setOnResumeCallback(SystemOnResumeCallback callback) override { onResumeCallback = callback; }
		void callOnResumeCallback();

		std::optional<String> getGameVersion() const override;

	private:
		WinRTPlatform* platform;
		int checkIfConnectedToTheInternetDelay;
		bool isConnectedToTheInternet;

		SystemOnSuspendCallback onSuspendCallback;
		SystemOnResumeCallback onResumeCallback;
	};

	class WinRTLocalSave : public ISaveData {
	public:
		WinRTLocalSave(String prefix);
		bool isReady() const override;
		Bytes getData(const String& path) override;
		std::vector<String> enumerate(const String& root) override;
		void setData(const String& path, const Bytes& data, bool commit) override;
		void removeData(const String& path) override;
		void commit() override;

	private:
		winrt::Windows::Storage::StorageFolder folder;
		static winrt::Windows::Storage::StorageFolder makeFolder(String prefix);
	};
}
