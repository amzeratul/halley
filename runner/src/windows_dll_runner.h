#pragma once

#ifdef _WIN32
#include "win.h"
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <boost/filesystem/path.hpp>

class WindowsDLLRunner : public IRunner
{
public:
	WindowsDLLRunner(std::string dll);
	~WindowsDLLRunner();
	void CheckForModifications();
	PrivateEngineData* initialize(std::vector<std::string> args) override;
	void terminate(PrivateEngineData* data) override;
	bool step(PrivateEngineData* data) override;

private:
	std::string binPath;
	std::string dllName;
	boost::filesystem::path libTempPath;

	HMODULE library;
	IRunner* runner;

	std::thread checkForRefreshThread;
	std::mutex refreshMutex;
	std::atomic<bool> hasRefresh = false;
	std::atomic<bool> waitingToDie = false;

	void RefreshLibrary();
};

#endif