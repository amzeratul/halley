#pragma once
#include "halley/resources/resource_data.h"
#include "halley/core/graphics/window.h"
#include "halley/concurrency/concurrent.h"

namespace Halley
{
	class VideoAPI;
	class InputAPI;
	class HalleyAPIInternal;

	class GLContext
	{
	public:
		virtual ~GLContext() {}
		virtual void bind() = 0;
		virtual std::unique_ptr<GLContext> createSharedContext() = 0;
	};

	enum class SaveDataType {
		Save,
		Cache
	};

	class SystemAPI
	{
	public:
		virtual ~SystemAPI() {}

		virtual Path getAssetsPath(const Path& gamePath) const = 0;
		virtual Path getUnpackedAssetsPath(const Path& gamePath) const = 0;

		virtual std::unique_ptr<ResourceDataReader> getDataReader(String path, int64_t start = 0, int64_t end = -1) = 0;
		
		virtual std::unique_ptr<GLContext> createGLContext() = 0;

		virtual std::shared_ptr<Window> createWindow(const WindowDefinition& window) = 0;
		virtual void destroyWindow(std::shared_ptr<Window> window) = 0;

		virtual Vector2i getScreenSize(int n) const = 0;
		virtual Rect4i getDisplayRect(int screen) const = 0;

		virtual void showCursor(bool show) = 0;

		virtual Bytes getSaveData(SaveDataType type, const String& path) = 0;
		virtual void setSaveData(SaveDataType type, const String& path, const Bytes& data) = 0;
		virtual std::vector<String> enumerateSaveData(SaveDataType type, const String& root) = 0;

		virtual std::thread createThread(const String& name, std::function<void()> runnable)
		{
			return std::thread([=] () {
				Concurrent::setThreadName(name);
				runnable();
			});
		}

		virtual void runGame(std::function<void()> runnable) { runnable(); }
		virtual bool canExit() { return false; }

	private:
		friend class HalleyAPI;
		friend class Core;

		virtual bool generateEvents(VideoAPI* video, InputAPI* input) = 0;
	};
}
