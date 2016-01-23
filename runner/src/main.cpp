#include "main.h"
#include "looper.h"
#include "windows_dll_runner.h"

int main(int argc, char** argv) {
	std::vector<std::string> args;
	for (int i = 0; i < argc; i++) {
		args.push_back(argv[i]);
	}

	WindowsDLLRunner runner("halleygame.dll");
	Looper looper;
	looper.Run(runner, args);

	return 0;
}
