#include "halley/file/directory_monitor.h"
#include "halley/support/exception.h"
#include "halley/file/path.h"

using namespace Halley;

#if defined(_WIN32) && !defined(WINDOWS_STORE)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Halley {
	class DirectoryMonitorPimpl
	{
	public:
		DirectoryMonitorPimpl(const Path& path)
			: path(path)
		{
			handle = FindFirstChangeNotification(path.string().c_str(), true, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
		}

		~DirectoryMonitorPimpl()
		{
			FindCloseChangeNotification(handle);
		}

		bool poll()
		{
			bool changed = false;
			while (true) {
				auto result = WaitForSingleObject(handle, 0);
				if (result == WAIT_OBJECT_0) {
					changed = true;
					FindNextChangeNotification(handle);
				} else if (result == WAIT_TIMEOUT) {
					break;
				} else if (result == WAIT_FAILED || WAIT_ABANDONED) {
					throw Exception("Failed to wait for object.", HalleyExceptions::Utils);
				}
			}
			return changed;
		}

		bool hasRealImplementation() const
		{
			return true;
		}

	private:
		HANDLE handle;
		Path path;
	};
}

#else

namespace Halley {
	// Not implemented
	class DirectoryMonitorPimpl
	{
	public:
		DirectoryMonitorPimpl(const Path&) {}
		bool poll() { return true; };
		bool hasRealImplementation() const { return false; }
	};
}

#endif

DirectoryMonitor::DirectoryMonitor(const Path& p)
	: pimpl(std::make_unique<DirectoryMonitorPimpl>(p))
{}

DirectoryMonitor::~DirectoryMonitor() = default;

bool DirectoryMonitor::poll()
{
	return pimpl->poll();
}

bool DirectoryMonitor::hasRealImplementation() const
{
	return pimpl->hasRealImplementation();
}
