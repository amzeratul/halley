#include "halley/file/directory_monitor.h"

#include "halley/support/exception.h"
#include "halley/file/path.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

using namespace Halley;

#if defined(_WIN32) && !defined(WINDOWS_STORE)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

namespace Halley {
	class DirectoryMonitorPimpl
	{
	public:
		DirectoryMonitorPimpl(const Path& path)
			: path(path)
		{
			dirHandle = CreateFileW(path.getNativeString().getUTF16().c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

			if (dirHandle != INVALID_HANDLE_VALUE) {
				buffer.resize(16 * 1024);

				ZeroMemory(&overlapped, sizeof(overlapped));
				overlapped.hEvent = CreateEvent(nullptr, false, 0, nullptr);

				queueEvent();
			}
		}

		~DirectoryMonitorPimpl()
		{
			if (dirHandle) {
				CloseHandle(overlapped.hEvent);
				CloseHandle(dirHandle);
			}
		}

		void poll(Vector<DirectoryMonitor::Event>& output, bool any)
		{
			if (!validHandle) {
				output.emplace_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::Unknown, {}, {} });
			}

			while (validHandle) {
				const auto result = WaitForSingleObject(overlapped.hEvent, 0);
				if (result == WAIT_OBJECT_0) {
					processEvents(output, any);
				} else if (result == WAIT_TIMEOUT) {
					break;
				} else if (result == WAIT_FAILED || WAIT_ABANDONED) {
					throw Exception("Failed to wait for object.", HalleyExceptions::Utils);
				}
			}
		}

		bool hasRealImplementation() const
		{
			return true;
		}

	private:
		HANDLE dirHandle = nullptr;
		Path path;
		OVERLAPPED overlapped;
		Vector<uint8_t> buffer;
		bool validHandle = false;

		void queueEvent()
		{
			validHandle = ReadDirectoryChangesW(dirHandle, buffer.data(), static_cast<DWORD>(buffer.size()), true,
						FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE, nullptr, &overlapped, nullptr);
		}

		void processEvents(Vector<DirectoryMonitor::Event>& output, bool any)
		{
			DWORD bytes;
			GetOverlappedResult(dirHandle, &overlapped, &bytes, false);
			if (bytes == 0 || any) {
				output.emplace_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::Unknown, {}, {} });
			} else {
				size_t pos = 0;

				while (true) {
					const auto* event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data() + pos);

					processEvent(*event, output);

					if (event->NextEntryOffset) {
						pos += event->NextEntryOffset;
					} else {
						break;
					}
				}
			}

			queueEvent();
		}

		void processEvent(const FILE_NOTIFY_INFORMATION& event, Vector<DirectoryMonitor::Event>& output)
		{
			const auto srcStr = std::wstring(event.FileName, event.FileNameLength / sizeof(wchar_t));
			const auto curPath = (path / Path(String(srcStr.c_str())));
			const auto nativePath = curPath.getNativeString().getUTF16();
			if (PathIsDirectoryW(nativePath.c_str())) {
				// Don't care
				return;
			}
			auto curPathStr = curPath.getString();

			switch (event.Action) {
			case FILE_ACTION_ADDED:
				output.emplace_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::FileAdded, std::move(curPathStr), {} });
				break;

			case FILE_ACTION_REMOVED:
				output.emplace_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::FileRemoved, std::move(curPathStr), {} });
				break;

			case FILE_ACTION_MODIFIED:
				output.emplace_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::FileModified, std::move(curPathStr), {} });
				break;

			case FILE_ACTION_RENAMED_OLD_NAME:
				output.emplace_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::FileRenamed, {}, std::move(curPathStr) });
				break;

			case FILE_ACTION_RENAMED_NEW_NAME:
				if (!output.empty() && output.back().type == DirectoryMonitor::ChangeType::FileRenamed) {
					output.back().name = std::move(curPathStr);
				}
				break;
			}
		}
	};
}

#else

namespace Halley {
	// Not implemented
	class DirectoryMonitorPimpl
	{
	public:
		DirectoryMonitorPimpl(const Path&) {}
		void poll(Vector<DirectoryMonitor::Event>& result, bool any)
		{
			result.emplace_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::Unknown, {}, {} });
		};
		bool hasRealImplementation() const { return false; }
	};
}

#endif

DirectoryMonitor::DirectoryMonitor(const Path& p)
	: pimpl(std::make_unique<DirectoryMonitorPimpl>(p))
{}

DirectoryMonitor::~DirectoryMonitor() = default;

void DirectoryMonitor::poll(Vector<Event>& output)
{
	pimpl->poll(output, false);
}

Vector<DirectoryMonitor::Event> DirectoryMonitor::poll()
{
	Vector<DirectoryMonitor::Event> result;
	pimpl->poll(result, false);
	return result;
}

bool DirectoryMonitor::pollAny()
{
	Vector<DirectoryMonitor::Event> result;
	pimpl->poll(result, true);
	return !result.empty();
}

bool DirectoryMonitor::hasRealImplementation() const
{
	return pimpl->hasRealImplementation();
}
