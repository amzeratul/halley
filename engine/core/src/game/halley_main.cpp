#include "halley/core/game/halley_main.h"
#include "halley/support/console.h"
#include "game/halley_main.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#endif

using namespace Halley;

int HalleyMain::runMain(GameLoader& loader, const Vector<std::string>& args)
{
	std::unique_ptr<Core> core;
	try {
		core = std::make_unique<Core>(loader.createGame(), args);
		loader.setCore(*core);
		core->init();
		MainLoop loop(*core, loader);
		loop.run();
		return core->getExitCode();
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

Vector<std::string> HalleyMain::getWin32Args()
{
	Vector<std::string> args;
#ifdef _WIN32
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
