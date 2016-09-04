#include "halley/runner/halley_main.h"
#include "halley/support/console.h"

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
		return 0;
	}
	catch (std::exception& e) {
		std::cout << ConsoleColour(Console::RED) << "\n\nUnhandled exception: " << ConsoleColour(Console::DARK_RED) << e.what() << ConsoleColour() << std::endl;
		core->onTerminatedInError();
		return 1;
	}
	catch (...) {
		std::cout << ConsoleColour(Console::RED) << "\n\nUnknown unhandled exception." << ConsoleColour() << std::endl;
		core->onTerminatedInError();
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
