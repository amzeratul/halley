#include "dynamic_loader.h"
#include <halley/runner/halley_main.h>

using namespace Halley;

int main(int argc, char** argv) {
	StringArray args;
	for (int i = 0; i < argc; i++) {
		args.push_back(argv[i]);
	}

	DynamicGameLoader loader("c:/dev/projects/halley/bin64/halley-sample-test");

	return HalleyMain::runMain(loader, args);
}
