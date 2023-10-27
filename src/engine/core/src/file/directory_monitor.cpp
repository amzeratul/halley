#include "halley/file/directory_monitor.h"

#include "halley/support/exception.h"
#include "halley/file/path.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

#include <thread>

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

			if (dirHandle == INVALID_HANDLE_VALUE) {
				dirHandle = nullptr;
			} else {
				buffer.resize(1024 * 1024);

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
			validHandle = ReadDirectoryChangesExW(dirHandle, buffer.data(), static_cast<DWORD>(buffer.size()), true,
						FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE, nullptr,
						&overlapped, nullptr, ReadDirectoryNotifyExtendedInformation);
		}

		void processEvents(Vector<DirectoryMonitor::Event>& output, bool any)
		{
			DWORD bytes;
			GetOverlappedResult(dirHandle, &overlapped, &bytes, true);
			if (bytes == 0 || any) {
				output.emplace_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::Unknown, {}, {} });
			} else {
				size_t pos = 0;

				while (true) {
					const auto* event = reinterpret_cast<FILE_NOTIFY_EXTENDED_INFORMATION*>(buffer.data() + pos);

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

		void processEvent(const FILE_NOTIFY_EXTENDED_INFORMATION& event, Vector<DirectoryMonitor::Event>& output)
		{
			const auto srcStr = std::wstring(event.FileName, event.FileNameLength / sizeof(wchar_t));
			const auto curPath = (path / Path(String(srcStr.c_str())));
			const auto nativePath = curPath.getNativeString().getUTF16();
			const bool isDir = event.FileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			auto curPathStr = curPath.getString();

			using CT = DirectoryMonitor::ChangeType;

			switch (event.Action) {
			case FILE_ACTION_ADDED:
				output.emplace_back(DirectoryMonitor::Event{ CT::FileAdded, isDir, std::move(curPathStr), {} });
				break;

			case FILE_ACTION_REMOVED:
				output.emplace_back(DirectoryMonitor::Event{ CT::FileRemoved, isDir, std::move(curPathStr), {} });
				break;

			case FILE_ACTION_MODIFIED:
				output.emplace_back(DirectoryMonitor::Event{ CT::FileModified, isDir, std::move(curPathStr), {} });
				break;

			case FILE_ACTION_RENAMED_OLD_NAME:
				output.emplace_back(DirectoryMonitor::Event{ CT::FileRenamed, isDir, {}, std::move(curPathStr) });
				break;

			case FILE_ACTION_RENAMED_NEW_NAME:
				if (!output.empty() && output.back().type == CT::FileRenamed) {
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

void DirectoryMonitor::poll(Vector<Event>& output, bool waitForNoChange)
{
	auto result = poll(waitForNoChange);
	output.reserve(output.size() + result.size());
	for (auto& e: result) {
		output.push_back(std::move(e));
	}
}

Vector<DirectoryMonitor::Event> DirectoryMonitor::poll(bool waitForNoChange)
{
	Vector<Event> result;

	size_t nBefore = 0;
	while (true) {
		pimpl->poll(result, false);
		const auto nNow = result.size();

		if (nNow != nBefore) {
			if (result.back().type == ChangeType::Unknown) {
				result.clear();
				return result;
			}
			if (waitForNoChange) {
				// Something got added, wait and try again
				nBefore = nNow;
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(100ms);
				continue;
			}
		}

		// If we get here, it means we don't need to wait for any more data
		if (!result.empty()) {
			postProcessEvents(result);
		}
		return result;
	}
}

bool DirectoryMonitor::pollAny()
{
	Vector<Event> result;
	pimpl->poll(result, true);
	return !result.empty();
}

bool DirectoryMonitor::hasRealImplementation() const
{
	return pimpl->hasRealImplementation();
}

void DirectoryMonitor::postProcessEvents(Vector<Event>& events)
{
	for (size_t i = 0; i < events.size(); ++i) {
		const auto& e = events[i];
		if (e.type == ChangeType::FileAdded || e.type == ChangeType::FileModified) {
			// Remove any file modified events of this afterwards
			for (size_t j = i + 1; j < events.size(); ) {
				if (events[j].type == ChangeType::FileModified && events[j].name == e.name) {
					events.erase(events.begin() + j);
				} else {
					++j;
				}
			}
		} else if (e.type == ChangeType::FileRemoved) {
			// Remove any references to this file before this
			const auto name = e.name;
			for (size_t j = 0; j < i; ) {
				if (events[j].type != ChangeType::FileRenamed && events[j].name == name) {
					events.erase(events.begin() + j);
					--i;
				} else {
					++j;
				}
			}
		} else if (e.type == ChangeType::FileRenamed) {
			// Remove any references to this file before or after
			const auto prevName = e.oldName;
			const auto name = e.name;
			for (size_t j = 0; j < i; ) {
				if (events[j].name == prevName) {
					events.erase(events.begin() + j);
					--i;
				} else {
					++j;
				}
			}
			for (size_t j = i + 1; j < events.size(); ) {
				if (events[j].type != ChangeType::FileRemoved && events[j].name == e.name) {
					events.erase(events.begin() + j);
				} else {
					++j;
				}
			}
		}
	}
}
