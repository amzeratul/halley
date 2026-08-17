#include "halley/game/halley_main.h"
#include "halley/game/core.h"
#include "halley/support/console.h"
#include "halley/utils/scoped_guard.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shellapi.h>
#endif

#if defined(NN_NINTENDO_SDK)
#include <nn/os.h>
#endif

using namespace Halley;

class SystemMainLoopHandler : public ISystemMainLoopHandler {
public:
	SystemMainLoopHandler(std::unique_ptr<Core> _core, std::unique_ptr<GameLoader> _loader)
		: core(std::move(_core))
		, loader(std::move(_loader))
		, loop(*core, *loader)
	{
	}

	~SystemMainLoopHandler() override
	{
	}

	bool run() override
	{
		try {
			if (!coreInit) {
				coreInit = true;
				core->init();
			}

			loop.runStep();
			return true;
		} catch (std::exception& e) {
			core->onTerminatedInError(e.what());
			return false;
		} catch (...) {
			core->onTerminatedInError("");
			return false;
		}
	}

private:
	std::unique_ptr<Core> core;
	std::unique_ptr<GameLoader> loader;
	MainLoop loop;
	bool coreInit = false;
};

namespace {
	int doRunMain(std::unique_ptr<Core>& core, std::unique_ptr<GameLoader> loader, const Vector<std::string>& args)
	{
		core = loader->createCore(args);
		loader->setCore(*core);
		auto* system = core->getAPI().system;

		if (system->mustOwnMainLoop()) {
			system->setGameLoopHandler(std::make_unique<SystemMainLoopHandler>(std::move(core), std::move(loader)));
			return 0;
		} else {
			system->runGame([&]() {
				core->init();
				MainLoop loop(*core, *loader);
				loop.run();
			});
		}

		return core->getExitCode();
	}
}

int HalleyMain::runMain(std::unique_ptr<GameLoader> loader, const Vector<std::string>& args)
{
	std::unique_ptr<Core> core;
	auto guard = ScopedGuard([&] ()
	{
		core->deInit();
	});

	if (Debug::canHandleUncaughtExceptions()) {
		return doRunMain(core, std::move(loader), args);
	} else {
		try {
			return doRunMain(core, std::move(loader), args);
		} catch (std::exception& e) {
			if (core) {
				core->onTerminatedInError(e.what());
			} else {
				std::cout << "Exception initialising core: " + String(e.what()) << std::endl;
			}
			return 1;
		} catch (...) {
			if (core) {
				core->onTerminatedInError("");
			} else {
				std::cout << "Unknown exception initialising core." << std::endl;
			}
			return 1;
		}
	}
}

Vector<std::string> HalleyMain::getWin32Args()
{
	Vector<std::string> args;
#if defined(_WIN32) && !defined(_GAMING_XBOX)
	auto cmd = GetCommandLineW();
	int argc;
	auto argv = CommandLineToArgvW(cmd, &argc);
	for (int i = 0; i < argc; i++) {
		args.push_back(String(reinterpret_cast<wchar_t**>(argv)[i]).cppStr());
	}
	LocalFree(argv);
#endif
	return args;
}

Vector<std::string> HalleyMain::getArgs(int argc, char* argv[])
{
#if defined(NN_NINTENDO_SDK)
	argc = nn::os::GetHostArgc();
	argv = nn::os::GetHostArgv();
#endif
	Vector<std::string> args;
	args.reserve(argc);
	for (int i = 0; i < argc; i++) {
		args.push_back(argv[i]);
	}
	return args;
}
