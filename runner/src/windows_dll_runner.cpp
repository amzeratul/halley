#include "windows_dll_runner.h"

#ifdef _WIN32
#include <iostream>
#include <boost/filesystem.hpp>

WindowsDLLRunner::WindowsDLLRunner(std::string dll)
	: binPath("C:/dev/halley2/runner/bin64/")
	, dllName(dll)
	, library(nullptr) // TODO: fixme
{
	RefreshLibrary();
	CheckForModifications();
}

WindowsDLLRunner::~WindowsDLLRunner()
{
	waitingToDie = true;
	checkForRefreshThread.join();
}

void WindowsDLLRunner::CheckForModifications()
{
	checkForRefreshThread = std::thread([&]() {
		using namespace boost::filesystem;
		auto dllPath = path(binPath + "/" + dllName);
		auto startTime = last_write_time(dllPath);

		while (!waitingToDie) {
			auto lastTime = last_write_time(dllPath);
			if (lastTime > startTime) {
				startTime = lastTime;
				std::unique_lock<std::mutex> m(refreshMutex);
				hasRefresh = true;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});
}

PrivateEngineData* WindowsDLLRunner::initialize(std::vector<std::string> args)
{
	return runner->initialize(args);
}

void WindowsDLLRunner::terminate(PrivateEngineData* data)
{
	runner->terminate(data);
}

bool WindowsDLLRunner::step(PrivateEngineData* data)
{
	bool needRefresh = false;
	{
		std::lock_guard<std::mutex> m(refreshMutex);
		needRefresh = hasRefresh;
		hasRefresh = false;
	}
	if (needRefresh) {
		RefreshLibrary();
	}

	runner->step(data);
	return true;
}

void WindowsDLLRunner::RefreshLibrary()
{
	using namespace boost::filesystem;

	// Clean up
	if (library != nullptr)	{
		if (runner != nullptr) {
			auto deleteRunner = reinterpret_cast<void(__stdcall*)(IRunner*)>(GetProcAddress(library, "HalleyDestroyRunner"));
			deleteRunner(runner);
			runner = nullptr;
		}
		FreeLibrary(library);
		library = nullptr;
		remove(libTempPath);
	}

	// Copy to temp
	libTempPath = unique_path("halleygame-%%%%-%%%%-%%%%-%%%%.dll");
	copy_file(path(binPath + "/" + dllName), libTempPath);

	// Load from temp
	library = LoadLibrary(libTempPath.string().c_str());
	if (library != nullptr) {
		auto getRunner = reinterpret_cast<IRunner*(__stdcall*)()>(GetProcAddress(library, "HalleyGetRunner"));
		runner = getRunner();
	}
}

#endif
