#pragma once
#include "halley/plugin/plugin.h"
#include "api/halley_api_internal.h"

namespace Halley {
	class DummySystemPlugin : public Plugin {
	public:
		PluginType getType() override;
		String getName() override;
		HalleyAPIInternal* createAPI(SystemAPI*) override;
		int getPriority() const override;
	};

	class DummySystemAPI : public SystemAPIInternal {
	public:
		std::unique_ptr<ResourceDataReader> getDataReader(String path, int64_t start, int64_t end) override;
		std::unique_ptr<ResourceDataReader> getDataReader(gsl::span<const gsl::byte> memory) override;
		std::unique_ptr<GLContext> createGLContext() override;
		std::shared_ptr<Window> createWindow(const WindowDefinition& window) override;
		void destroyWindow(std::shared_ptr<Window> window) override;
		Vector2i getScreenSize(int n) const override;
		Rect4i getDisplayRect(int screen) const override;
		void showCursor(bool show) override;
	private:
		bool generateEvents(VideoAPI* video, InputAPI* input) override;
	public:
		void init() override;
		void deInit() override;
	};
}
