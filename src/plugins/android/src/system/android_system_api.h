#pragma once

#include "halley/core/api/halley_api_internal.h"
#include <EGL/egl.h>

namespace Halley {
    class AndroidSystemAPI : public SystemAPIInternal {
    public:
        void init() override;
        void deInit() override;

		Path getAssetsPath(const Path& gamePath) const override;
		Path getUnpackedAssetsPath(const Path& gamePath) const override;

		std::unique_ptr<ResourceDataReader> getDataReader(String path, int64_t start = 0, int64_t end = -1) override;
		
		std::unique_ptr<GLContext> createGLContext() override;

		std::shared_ptr<Window> createWindow(const WindowDefinition& window) override;
		void destroyWindow(std::shared_ptr<Window> window) override;

		Vector2i getScreenSize(int n) const override;
		Rect4i getDisplayRect(int screen) const override;

		void showCursor(bool show) override;

		std::shared_ptr<ISaveData> getStorageContainer(SaveDataType type, const String& containerName = "") override;

		bool generateEvents(VideoAPI* video, InputAPI* input) override;

    private:
        EGLConfig config;
		EGLDisplay display;
		EGLSurface surface;
    };
}
