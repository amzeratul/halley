#include "halley/file/directory_monitor.h"

#include "halley/support/exception.h"
#include "halley/file/path.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

#include <optional>
#include <thread>

using namespace Halley;

#if defined(_WIN32) && !defined(WITH_GDK)

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
				Logger::logError("Directory monitor could not be set up for " + path.toString());
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
				} else if (result == WAIT_FAILED || result == WAIT_ABANDONED) {
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
		bool useExtended = true;

		void queueEvent()
		{
			validHandle = tryQueueEvent();
		}

		bool tryQueueEvent()
		{
			constexpr DWORD notifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;

			if (useExtended) {
				if (ReadDirectoryChangesExW(dirHandle, buffer.data(), static_cast<DWORD>(buffer.size()), true, notifyFilter, nullptr, &overlapped, nullptr, ReadDirectoryNotifyExtendedInformation)) {
					return true;
				}

				// ReFS doesn't support ReadDirectoryChangesExW with extended information
				Logger::logWarning("ReadDirectoryChangesExW failed for " + path.toString() + " (error " + toString(static_cast<int>(GetLastError())) + "); falling back to non-extended directory monitoring.");
				useExtended = false;
			}

			if (ReadDirectoryChangesW(dirHandle, buffer.data(), static_cast<DWORD>(buffer.size()), true, notifyFilter, nullptr, &overlapped, nullptr)) {
				return true;
			}

			Logger::logError("ReadDirectoryChangesW failed for " + path.toString() + " (error " + toString(static_cast<int>(GetLastError())) + ").");
			return false;
		}

		void processEvents(Vector<DirectoryMonitor::Event>& output, bool any)
		{
			DWORD bytes;
			GetOverlappedResult(dirHandle, &overlapped, &bytes, true);
			if (bytes > 0) {
				if (any) {
					output.emplace_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::Unknown, {}, {} });
				} else if (useExtended) {
					size_t pos = 0;

					while (true) {
						const auto* event = reinterpret_cast<FILE_NOTIFY_EXTENDED_INFORMATION*>(buffer.data() + pos);

						const bool isDir = (event->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
						processEvent(event->Action, event->FileName, event->FileNameLength, isDir, output);

						if (event->NextEntryOffset) {
							pos += event->NextEntryOffset;
						} else {
							break;
						}
					}
				} else {
					size_t pos = 0;

					while (true) {
						const auto* event = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data() + pos);

						processEvent(event->Action, event->FileName, event->FileNameLength, std::nullopt, output);

						if (event->NextEntryOffset) {
							pos += event->NextEntryOffset;
						} else {
							break;
						}
					}
				}
			}

			queueEvent();
		}

		void processEvent(DWORD action, const wchar_t* fileName, DWORD fileNameLength, std::optional<bool> isDirHint, Vector<DirectoryMonitor::Event>& output)
		{
			const auto srcStr = std::wstring(fileName, fileNameLength / sizeof(wchar_t));
			const auto curPath = (path / Path(String(srcStr.c_str())));
			auto curPathStr = curPath.getString();

			bool isDir;
			if (isDirHint) {
				isDir = *isDirHint;
			} else {
				// Non-extended notifications don't carry attributes; query the filesystem
				const auto attribs = GetFileAttributesW(curPath.getNativeString().getUTF16().c_str());
				isDir = attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
			}

			using CT = DirectoryMonitor::ChangeType;

			switch (action) {
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

void DirectoryMonitor::poll(Vector<Event>& output, const DelayRules& rules)
{
	auto result = poll(rules);
	output.reserve(output.size() + result.size());
	for (auto& e: result) {
		output.push_back(std::move(e));
	}
}

namespace {
	int getDelayMs(const DirectoryMonitor::DelayRules& rules, const Vector<DirectoryMonitor::Event>& events)
	{
		int delay = rules.baseDelay;
		for (const auto& r: rules.rules) {
			if (r.msDelay > delay) {
				for (const auto& e: events) {
					if ((e.type == DirectoryMonitor::ChangeType::FileAdded || e.type == DirectoryMonitor::ChangeType::FileModified)
						&& e.name.endsWith(r.suffix)) {
						delay = r.msDelay;
						break;
					}
				}
			}
		}
		return delay;
	}
}

Vector<DirectoryMonitor::Event> DirectoryMonitor::poll(const DelayRules& rules)
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
			
			if (auto delay = getDelayMs(rules, result); delay > 0) {
				// Something got added, wait and try again
				nBefore = nNow;
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(delay * 1ms);
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
