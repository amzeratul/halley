#pragma once
#include "api/halley_api_internal.h"

namespace Halley {
	class DummySystemAPI : public SystemAPIInternal {
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
		bool generateEvents(VideoAPI* video, InputAPI* input) override;

		std::shared_ptr<ISaveData> getStorageContainer(SaveDataType type, const String& containerName) override;
	};

	class DummySaveData : public ISaveData {
	public:
		bool isReady() const override;
		Bytes getData(const String& path) override;
		std::vector<String> enumerate(const String& root) override;
		void setData(const String& path, const Bytes& data) override;
		void commit() override;
	};

	class DummyWindow : public Window
	{
	public:
		explicit DummyWindow(const WindowDefinition& definition);

		void update(const WindowDefinition& definition) override;
		void show() override;
		void hide() override;
		void setVsync(bool vsync) override;
		void swap() override;
		Rect4i getWindowRect() const override;
		const WindowDefinition& getDefinition() const override;

	private:
		const WindowDefinition& definition;
	};
}
