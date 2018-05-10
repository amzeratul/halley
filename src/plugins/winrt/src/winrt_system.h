#pragma once
#include "halley/core/api/halley_api_internal.h"

namespace Halley {
	class WinRTSystem : public SystemAPIInternal {
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

		Bytes getSaveData(SaveDataType type, const String& path) override;
		void setSaveData(SaveDataType type, const String& path, const Bytes& data) override;
		std::vector<String> enumerateSaveData(SaveDataType type, const String& root) override;

		bool generateEvents(VideoAPI* video, InputAPI* input) override;

		void runGame(std::function<void()> runnable) override;
	};
}
